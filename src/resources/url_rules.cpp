#include <bitset>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#include "resources/url_rules.hpp"

#include "diagnostics/diagnostics.hpp"
#include "diagnostics/string_view.hpp"
#include "runtime/runtime.hpp"

namespace mln::core {
namespace {

constexpr auto known_url_match_flags =
  static_cast<std::uint32_t>(MLN_URL_MATCH_GLOB);

auto reject(const char* what, std::size_t index, const char* reason)
  -> mln_status {
  set_thread_error(
    (std::string{what} + " " + std::to_string(index) + " " + reason).c_str()
  );
  return MLN_STATUS_INVALID_ARGUMENT;
}

// Returns the element at cursor and advances cursor past it by the element's
// declared size, so a host built against a later header, whose elements carry
// members this library does not know, steps over them. Null when the element
// is smaller than this library's struct or its size would misalign the next.
template <typename Element>
auto next_sized_element(const std::byte*& cursor) -> const Element* {
  const auto* element = reinterpret_cast<const Element*>(cursor);
  if (
    element->size < sizeof(Element) || element->size % alignof(Element) != 0
  ) {
    return nullptr;
  }
  cursor += element->size;
  return element;
}

// Checks the members that routes and header rules share.
auto check_pattern(
  const char* what, std::size_t index, std::uint32_t kinds, std::uint32_t flags,
  mln_buffer_view url
) -> mln_status {
  if (!validate_c_string_view(url, "url"))
    return reject(what, index, "has an invalid url");
  if ((kinds & ~MLN_RESOURCE_KIND_FLAG_ALL) != 0) {
    return reject(what, index, "has unknown kind bits");
  }
  if ((flags & ~known_url_match_flags) != 0) {
    return reject(what, index, "has unknown flag bits");
  }
  if (
    (flags & MLN_URL_MATCH_GLOB) != 0 &&
    CompiledGlob::token_count(string_view_from_view(url)) >
      CompiledGlob::max_tokens
  ) {
    return reject(what, index, "exceeds the glob limit of 4096 tokens");
  }
  return MLN_STATUS_OK;
}

}  // namespace

auto CompiledGlob::token_count(std::string_view pattern) noexcept
  -> std::size_t {
  auto count = std::size_t{0};
  for (auto index = std::size_t{0}; index < pattern.size(); ++index) {
    if (pattern[index] == '*') {
      while (index + 1 < pattern.size() && pattern[index + 1] == '*') ++index;
    } else if (pattern[index] == '\\' && index + 1 < pattern.size()) {
      ++index;
    }
    ++count;
  }
  return count;
}

CompiledGlob::CompiledGlob(std::string_view pattern) {
  const auto count = token_count(pattern);
  if (count > max_tokens) throw std::length_error{"glob exceeds 4096 tokens"};
  tokens_.reserve(count);
  for (auto index = std::size_t{0}; index < pattern.size(); ++index) {
    const auto value = pattern[index];
    if (value == '*') {
      const auto start = index;
      while (index + 1 < pattern.size() && pattern[index + 1] == '*') ++index;
      tokens_.push_back({index == start ? Kind::Segment : Kind::Spanning});
    } else if (value == '?') {
      tokens_.push_back({Kind::Any});
    } else {
      if (value == '\\' && index + 1 < pattern.size()) ++index;
      tokens_.push_back({Kind::Literal, pattern[index]});
    }
  }
}

auto CompiledGlob::matches(std::string_view candidate) const noexcept -> bool {
  auto states = std::bitset<max_tokens + 1>{};
  states[0] = 1;
  for (auto index = std::size_t{0}; index < tokens_.size(); ++index) {
    const auto kind = tokens_[index].kind;
    states[index + 1] =
      (kind == Kind::Segment || kind == Kind::Spanning) && states[index] != 0;
  }
  for (const auto byte : candidate) {
    auto previous = static_cast<bool>(states[0]);
    states[0] = 0;
    for (auto index = std::size_t{0}; index < tokens_.size(); ++index) {
      const auto old = static_cast<bool>(states[index + 1]);
      const auto token = tokens_[index];
      switch (token.kind) {
        case Kind::Segment:
        case Kind::Spanning:
          states[index + 1] =
            states[index] != 0 ||
            (old != 0 && (token.kind == Kind::Spanning || byte != '/'));
          break;
        case Kind::Any:
          states[index + 1] = previous != 0 && byte != '/';
          break;
        case Kind::Literal:
          states[index + 1] = previous != 0 && byte == token.literal;
          break;
      }
      previous = old;
    }
  }
  return states[tokens_.size()] != 0;
}

// Zero kinds select every kind.
UrlPattern::UrlPattern(
  std::uint32_t kinds_, std::uint32_t flags, std::string_view url_
)
    : kinds(kinds_ == 0 ? MLN_RESOURCE_KIND_FLAG_ALL : kinds_),
      url(url_),
      glob(
        (flags & MLN_URL_MATCH_GLOB) != 0 ? std::make_shared<CompiledGlob>(url_)
                                          : nullptr
      ) {}

auto UrlPattern::matches(
  mln_resource_kind request_kind, std::string_view candidate
) const noexcept -> bool {
  return matches_kind(kinds, request_kind) &&
         (glob != nullptr ? glob->matches(candidate) : url == candidate);
}

auto RouteTable::matches(
  mln_resource_kind kind, std::string_view resolved_url
) const noexcept -> bool {
  if (routes.empty()) return true;
  for (const auto& route : routes) {
    if (route.matches(kind, resolved_url)) {
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
  const auto* cursor = reinterpret_cast<const std::byte*>(routes);
  for (std::size_t index = 0; index < count; ++index) {
    const auto* element = next_sized_element<mln_resource_route>(cursor);
    if (element == nullptr)
      return reject("route", index, "has an invalid size");
    const auto& route = *element;
    const auto status =
      check_pattern("route", index, route.kinds, route.flags, route.url);
    if (status != MLN_STATUS_OK) return status;
    out.routes.emplace_back(
      route.kinds, route.flags, string_view_from_view(route.url)
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
  const auto* cursor = reinterpret_cast<const std::byte*>(rules);
  for (std::size_t index = 0; index < count; ++index) {
    const auto* element = next_sized_element<mln_http_header_rule>(cursor);
    if (element == nullptr) {
      return reject("header rule", index, "has an invalid size");
    }
    const auto& rule = *element;
    const auto status =
      check_pattern("header rule", index, rule.kinds, rule.flags, rule.url);
    if (status != MLN_STATUS_OK) return status;
    if (rule.headers == nullptr && rule.header_count != 0) {
      return reject(
        "header rule", index, "has null headers with a nonzero header_count"
      );
    }
    auto copied = HeaderRuleTable::Rule{
      .pattern = {rule.kinds, rule.flags, string_view_from_view(rule.url)},
      .headers = {},
    };
    copied.headers.reserve(rule.header_count);
    for (std::size_t header = 0; header < rule.header_count; ++header) {
      const auto name = rule.headers[header].name;
      const auto value = rule.headers[header].value;
      // The validation diagnostic names the header, never its value.
      if (
        validate_http_header(
          static_cast<const char*>(name.data), name.size,
          static_cast<const char*>(value.data), value.size
        ) != MLN_STATUS_OK
      ) {
        set_thread_error(("header rule " + std::to_string(index) + ": " +
                          thread_last_error_message())
                           .c_str());
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      copied.headers.emplace_back(
        string_from_view(name), string_from_view(value)
      );
    }
    out.rules.push_back(std::move(copied));
  }
  return MLN_STATUS_OK;
}

auto header_rules_callback(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_http_header_transform_response* out_response
) -> mln_status {
  if (user_data == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }
  const auto& table = *static_cast<const HeaderRuleTable*>(user_data);
  const auto candidate =
    std::string_view{static_cast<const char*>(url.data), url.size};
  for (const auto& rule : table.rules) {
    if (!rule.pattern.matches(kind, candidate)) continue;
    for (const auto& [name, value] : rule.headers) {
      const auto status = http_header_transform_response_set(
        out_response, {.data = name.data(), .size = name.size()},
        {.data = value.data(), .size = value.size()}
      );
      if (status != MLN_STATUS_OK) return status;
    }
    return MLN_STATUS_OK;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
