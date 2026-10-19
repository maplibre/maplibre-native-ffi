#include <cstring>
#include <string>
#include <string_view>

#include "resources/url_rules.hpp"

#include "diagnostics/diagnostics.hpp"
#include "runtime/runtime.hpp"

namespace mln::core {
namespace {

constexpr auto known_url_match_flags =
  static_cast<std::uint32_t>(MLN_URL_MATCH_GLOB) |
  static_cast<std::uint32_t>(MLN_URL_MATCH_REQUESTED_URL);

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

auto reject(const char* what, std::size_t index, const char* reason)
  -> mln_status {
  set_thread_error(
    (std::string{what} + " " + std::to_string(index) + " " + reason).c_str()
  );
  return MLN_STATUS_INVALID_ARGUMENT;
}

// Checks the members that routes and rules share. allowed names the flags
// that this kind of entry accepts.
auto check_pattern(
  const char* what, std::size_t index, std::uint32_t flags,
  std::uint32_t allowed, const char* url
) -> mln_status {
  if (url == nullptr) return reject(what, index, "has a null url");
  if ((flags & ~known_url_match_flags) != 0) {
    return reject(what, index, "has unknown flag bits");
  }
  if ((flags & ~allowed) != 0) {
    return reject(what, index, "sets MLN_URL_MATCH_REQUESTED_URL");
  }
  return MLN_STATUS_OK;
}

}  // namespace

auto glob_matches(std::string_view pattern, std::string_view candidate) noexcept
  -> bool {
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

auto url_matches(
  std::uint32_t flags, std::string_view url, std::string_view candidate
) noexcept -> bool {
  if ((flags & static_cast<std::uint32_t>(MLN_URL_MATCH_GLOB)) != 0) {
    return glob_matches(url, candidate);
  }
  return url == candidate;
}

auto UrlPattern::matches(
  std::uint32_t request_kind, std::string_view candidate
) const noexcept -> bool {
  return matches_kind(kind, request_kind) && url_matches(flags, url, candidate);
}

auto RouteTable::matches(
  std::uint32_t kind, std::string_view requested_url,
  std::string_view resolved_url
) const noexcept -> bool {
  if (routes.empty()) return true;
  for (const auto& route : routes) {
    const auto requested =
      (route.flags & static_cast<std::uint32_t>(MLN_URL_MATCH_REQUESTED_URL)) !=
      0;
    if (route.matches(kind, requested ? requested_url : resolved_url)) {
      return true;
    }
  }
  return false;
}

auto copy_routes(
  const mln_resource_route* routes, std::size_t count, RouteTable& out
) -> mln_status {
  if (routes == nullptr && count != 0) {
    set_thread_error("routes must not be null with a nonzero route_count");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out.routes.clear();
  out.routes.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    const auto& route = routes[index];
    const auto status = check_pattern(
      "route", index, route.flags, known_url_match_flags, route.url
    );
    if (status != MLN_STATUS_OK) return status;
    out.routes.push_back({route.kind, route.flags, route.url});
  }
  return MLN_STATUS_OK;
}

auto copy_rewrite_rules(
  const mln_resource_rewrite_rule* rules, std::size_t count,
  RewriteRuleTable& out
) -> mln_status {
  if (rules == nullptr && count != 0) {
    set_thread_error("rules must not be null with a nonzero rule_count");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out.rules.clear();
  out.rules.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    const auto& rule = rules[index];
    const auto status = check_pattern(
      "rewrite rule", index, rule.flags, MLN_URL_MATCH_GLOB, rule.url
    );
    if (status != MLN_STATUS_OK) return status;
    out.rules.push_back(
      RewriteRuleTable::Rule{
        .pattern = {rule.kind, rule.flags, rule.url},
        .replacement_url = rule.replacement_url == nullptr
                             ? std::nullopt
                             : std::optional<std::string>{rule.replacement_url},
      }
    );
  }
  return MLN_STATUS_OK;
}

auto copy_header_rules(
  const mln_http_header_rule* rules, std::size_t count, HeaderRuleTable& out
) -> mln_status {
  if (rules == nullptr && count != 0) {
    set_thread_error("rules must not be null with a nonzero rule_count");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out.rules.clear();
  out.rules.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    const auto& rule = rules[index];
    const auto status = check_pattern(
      "header rule", index, rule.flags, MLN_URL_MATCH_GLOB, rule.url
    );
    if (status != MLN_STATUS_OK) return status;
    if (rule.headers == nullptr && rule.header_count != 0) {
      return reject(
        "header rule", index, "has null headers with a nonzero header_count"
      );
    }
    auto copied = HeaderRuleTable::Rule{
      .pattern = {rule.kind, rule.flags, rule.url},
      .headers = {},
    };
    copied.headers.reserve(rule.header_count);
    for (std::size_t header = 0; header < rule.header_count; ++header) {
      const auto* name = rule.headers[header].name;
      const auto* value = rule.headers[header].value;
      if (name == nullptr || value == nullptr) {
        return reject(
          "header rule", index, "has a header with a null name or value"
        );
      }
      // The validation diagnostic names the header, never its value.
      if (
        validate_http_header(
          name, std::strlen(name), value, std::strlen(value)
        ) != MLN_STATUS_OK
      ) {
        set_thread_error(("header rule " + std::to_string(index) + ": " +
                          thread_last_error_message())
                           .c_str());
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      copied.headers.emplace_back(name, value);
    }
    out.rules.push_back(std::move(copied));
  }
  return MLN_STATUS_OK;
}

auto rewrite_rules_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) noexcept -> mln_status {
  if (user_data == nullptr || url == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }
  const auto& table = *static_cast<const RewriteRuleTable*>(user_data);
  for (const auto& rule : table.rules) {
    if (!rule.pattern.matches(kind, url)) continue;
    if (!rule.replacement_url.has_value()) return MLN_STATUS_OK;
    return mln_resource_transform_response_set_url(
      out_response, rule.replacement_url->data(), rule.replacement_url->size(),
      nullptr
    );
  }
  return MLN_STATUS_OK;
}

auto header_rules_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) noexcept -> mln_status {
  if (user_data == nullptr || url == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }
  const auto& table = *static_cast<const HeaderRuleTable*>(user_data);
  for (const auto& rule : table.rules) {
    if (!rule.pattern.matches(kind, url)) continue;
    for (const auto& [name, value] : rule.headers) {
      const auto status = mln_http_header_transform_response_set_header(
        out_response, name.data(), name.size(), value.data(), value.size(),
        nullptr
      );
      if (status != MLN_STATUS_OK) return status;
    }
    return MLN_STATUS_OK;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
