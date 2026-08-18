#define MLN_BUILDING_C

// Adapts synchronous MapLibre callback contracts to hosts that can only receive
// callbacks asynchronously through void listener functions.
//
// Everything here runs on MapLibre's own threads.

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "maplibre_native_c/callback_adapter.h"

#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"

namespace {

using AdapterResourceRewriteRules = mln_adapter_resource_rewrite_rules;
using AdapterHttpHeaderTransformRules = mln_adapter_http_header_transform_rules;
using AdapterResourceProviderRules = mln_adapter_resource_provider_rules;
using AdapterQueuedResourceProviderRoute =
  mln_adapter_queued_resource_provider_route;
using AdapterQueuedResourceProvider = mln_adapter_queued_resource_provider;
using AdapterQueuedResourceRequestView = mln_adapter_queued_resource_request;

struct AdapterQueuedResourceRequest {
  AdapterQueuedResourceRequestView view{};
  std::string requested_url;
  std::string resolved_url;
  std::string prior_etag;
  std::vector<std::uint8_t> prior_data;
};

using AdapterLogCallbackState = mln_adapter_log_callback_state;
using AdapterLogRecordView = mln_adapter_log_record;

struct AdapterLogRecord {
  AdapterLogRecordView view{};
  std::string message;
};

struct AdapterLogCallbackEntry {
  mln_adapter_log_record_listener listener = nullptr;
  std::uint32_t consume = 0;
  std::size_t in_flight = 0;
  bool retired = false;
};

struct AdapterHandleLeakToken {
  std::string type_name;
  std::uint64_t handle = 0;
};

std::mutex log_setter_mutex;
std::mutex log_state_mutex;
std::unordered_map<void*, AdapterLogCallbackEntry> log_callbacks;
void* active_log_callback = nullptr;

auto matches_rule(std::uint32_t rule_kind, std::uint32_t request_kind) -> bool {
  return rule_kind == MLN_ADAPTER_RESOURCE_KIND_ANY ||
         rule_kind == request_kind;
}

// Returns how many pattern characters the element at `index` spans, or zero
// when it does not match `candidate`.
auto literal_span(std::string_view pattern, std::size_t index, char candidate)
  -> std::size_t {
  const auto element = pattern[index];
  if (element == '?') {
    return candidate == '/' ? 0 : 1;
  }
  if (element == '\\' && index + 1 < pattern.size()) {
    return pattern[index + 1] == candidate ? 2 : 0;
  }
  return element == candidate ? 1 : 0;
}

// Matches one glob pattern against a candidate URL, in the language
// callback_adapter.h documents. A '*' run stops at a '/' unless the pattern
// spelled '**'. Matching is iterative and allocation-free so that a host
// pattern cannot overflow the MapLibre thread this runs on; the worst case
// costs the product of the two lengths.
auto glob_matches(std::string_view pattern, std::string_view candidate)
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

constexpr auto KnownUrlMatchFlags =
  static_cast<std::uint32_t>(MLN_ADAPTER_URL_MATCH_GLOB);

constexpr auto KnownRouteFlags =
  static_cast<std::uint32_t>(MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB) |
  static_cast<std::uint32_t>(MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL);

static_assert(
  static_cast<std::uint32_t>(MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB) ==
    static_cast<std::uint32_t>(MLN_ADAPTER_URL_MATCH_GLOB),
  "a queued provider route selects glob matching with the shared flag bit"
);

// A null url, an absent candidate, or a flag bit outside known_flags matches
// nothing rather than everything.
auto url_matches(
  std::uint32_t flags, std::uint32_t known_flags, const char* url,
  const char* candidate
) -> bool {
  if (url == nullptr || candidate == nullptr || (flags & ~known_flags) != 0) {
    return false;
  }
  const auto pattern = std::string_view{url};
  const auto target = std::string_view{candidate};
  if ((flags & static_cast<std::uint32_t>(MLN_ADAPTER_URL_MATCH_GLOB)) != 0) {
    return glob_matches(pattern, target);
  }
  return pattern == target;
}

auto has_flag(std::uint32_t flags, mln_adapter_resource_route_flags flag)
  -> bool {
  return (flags & static_cast<std::uint32_t>(flag)) != 0;
}

auto route_matches_url(
  const AdapterQueuedResourceProviderRoute& route,
  const mln_resource_request& request
) -> bool {
  const auto* candidate =
    has_flag(route.flags, MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL)
      ? request.requested_url
      : request.resolved_url;
  return url_matches(route.flags, KnownRouteFlags, route.url, candidate);
}

auto request_matches_route(
  std::span<const AdapterQueuedResourceProviderRoute> routes,
  const mln_resource_request& request
) -> bool {
  return std::ranges::any_of(routes, [&request](const auto& route) -> bool {
    return matches_rule(route.kind, request.kind) &&
           route_matches_url(route, request);
  });
}

auto copy_prior_data(const mln_resource_request& request)
  -> std::vector<std::uint8_t> {
  if (request.prior_data == nullptr || request.prior_data_size == 0) {
    return {};
  }
  auto data = std::vector<std::uint8_t>{};
  data.resize(request.prior_data_size);
  std::ranges::copy(
    std::span{request.prior_data, request.prior_data_size}, data.begin()
  );
  return data;
}

auto copy_request(
  const mln_resource_request& request, mln_resource_request_handle handle
) -> AdapterQueuedResourceRequestView* {
  auto copy = std::make_unique<AdapterQueuedResourceRequest>();
  copy->requested_url = request.requested_url == nullptr
                          ? std::string{}
                          : std::string{request.requested_url};
  copy->resolved_url = request.resolved_url == nullptr
                         ? std::string{}
                         : std::string{request.resolved_url};
  copy->prior_etag = request.prior_etag == nullptr
                       ? std::string{}
                       : std::string{request.prior_etag};
  copy->prior_data = copy_prior_data(request);
  copy->view = AdapterQueuedResourceRequestView{
    .owner = copy.get(),
    .handle = handle,
    .requested_url = copy->requested_url.c_str(),
    .resolved_url = copy->resolved_url.c_str(),
    .kind = request.kind,
    .loading_method = request.loading_method,
    .priority = request.priority,
    .usage = request.usage,
    .storage_policy = request.storage_policy,
    .has_range = request.has_range,
    .range_start = request.range_start,
    .range_end = request.range_end,
    .has_prior_modified = request.has_prior_modified,
    .prior_modified_unix_ms = request.prior_modified_unix_ms,
    .has_prior_expires = request.has_prior_expires,
    .prior_expires_unix_ms = request.prior_expires_unix_ms,
    .prior_etag = copy->prior_etag.empty() ? nullptr : copy->prior_etag.c_str(),
    .prior_data = copy->prior_data.empty() ? nullptr : copy->prior_data.data(),
    .prior_data_size = copy->prior_data.size(),
  };
  return &copy.release()->view;
}

void destroy_queued_request(
  AdapterQueuedResourceRequestView* request
) noexcept {
  if (request == nullptr) {
    return;
  }
  auto* owner = static_cast<AdapterQueuedResourceRequest*>(request->owner);
  static_cast<void>(std::unique_ptr<AdapterQueuedResourceRequest>{owner});
}

auto copy_log_record(
  std::uint32_t severity, std::uint32_t event, std::int64_t code,
  const char* message, bool retire_callback = false
) -> AdapterLogRecordView* {
  auto copy = std::make_unique<AdapterLogRecord>();
  copy->message = message == nullptr ? std::string{} : std::string{message};
  copy->view = AdapterLogRecordView{
    .owner = copy.get(),
    .retire_callback = retire_callback,
    .severity = severity,
    .event = event,
    .code = code,
    .message = copy->message.c_str(),
  };
  return &copy.release()->view;
}

void destroy_log_record(AdapterLogRecordView* record) noexcept {
  if (record == nullptr) {
    return;
  }
  auto* owner = static_cast<AdapterLogRecord*>(record->owner);
  static_cast<void>(std::unique_ptr<AdapterLogRecord>{owner});
}

void queue_log_retirement(mln_adapter_log_record_listener listener) noexcept {
  if (listener == nullptr) {
    return;
  }
  try {
    listener(nullptr);
  } catch (...) {
    // Listener delivery is notification-only at this boundary.
  }
}

void destroy_handle_leak_token(void* token) noexcept {
  static_cast<void>(std::unique_ptr<AdapterHandleLeakToken>{
    static_cast<AdapterHandleLeakToken*>(token),
  });
}

}  // namespace

extern "C" MLN_API auto mln_adapter_handle_leak_token_create(
  const char* type_name, std::uint64_t handle
) noexcept -> void* {
  try {
    auto token = std::make_unique<AdapterHandleLeakToken>();
    token->type_name = type_name == nullptr ? std::string{} : type_name;
    token->handle = handle;
    return token.release();
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API void mln_adapter_handle_leak_token_destroy(
  void* token
) noexcept {
  destroy_handle_leak_token(token);
}

extern "C" MLN_API void mln_adapter_handle_leak_report(void* token) noexcept {
  auto* leak = static_cast<AdapterHandleLeakToken*>(token);
  if (leak != nullptr) {
    static_cast<void>(std::fputs("maplibre_native_ffi: leaked ", stderr));
    static_cast<void>(std::fputs(leak->type_name.c_str(), stderr));
    static_cast<void>(std::fprintf(
      stderr, " handle %llu", static_cast<unsigned long long>(leak->handle)
    ));
    static_cast<void>(std::fputs(
      "; close it from its owning execution context before releasing the host "
      "object\n",
      stderr
    ));
  }
  destroy_handle_leak_token(token);
}

extern "C" MLN_API auto mln_adapter_log_callback(
  void* user_data, std::uint32_t severity, std::uint32_t event,
  std::int64_t code, const char* message
) noexcept -> std::uint32_t {
  if (user_data == nullptr) {
    return 0;
  }
  mln_adapter_log_record_listener listener = nullptr;
  std::uint32_t consume = 0;
  {
    const auto lock = std::scoped_lock{log_state_mutex};
    const auto iterator = log_callbacks.find(user_data);
    if (iterator == log_callbacks.end() || iterator->second.retired) {
      return 0;
    }
    listener = iterator->second.listener;
    consume = iterator->second.consume;
    ++iterator->second.in_flight;
  }
  if (listener != nullptr) {
    try {
      listener(copy_log_record(severity, event, code, message));
    } catch (...) {
      // Logging callbacks are notification-only at the host boundary.
    }
  }
  mln_adapter_log_record_listener retirement_listener = nullptr;
  {
    const auto lock = std::scoped_lock{log_state_mutex};
    const auto iterator = log_callbacks.find(user_data);
    if (iterator != log_callbacks.end()) {
      --iterator->second.in_flight;
      if (iterator->second.retired && iterator->second.in_flight == 0) {
        retirement_listener = iterator->second.listener;
        log_callbacks.erase(iterator);
      }
    }
  }
  queue_log_retirement(retirement_listener);
  return consume;
}

extern "C" MLN_API auto mln_adapter_log_set_callback(
  mln_adapter_log_callback_state* state
) noexcept -> mln_status {
  const auto setter_lock = std::scoped_lock{log_setter_mutex};
  if (state != nullptr) {
    const auto state_lock = std::scoped_lock{log_state_mutex};
    log_callbacks[state] = AdapterLogCallbackEntry{
      .listener = state->listener,
      .consume = state->consume,
    };
  }
  const auto status = state == nullptr
                        ? mln_log_clear_callback()
                        : mln_log_set_callback(mln_adapter_log_callback, state);
  if (status != MLN_STATUS_OK) {
    if (state != nullptr) {
      const auto state_lock = std::scoped_lock{log_state_mutex};
      log_callbacks.erase(state);
    }
    return status;
  }
  mln_adapter_log_record_listener retirement_listener = nullptr;
  {
    const auto state_lock = std::scoped_lock{log_state_mutex};
    auto* retired_state = active_log_callback;
    active_log_callback = state;
    if (retired_state != nullptr && retired_state != state) {
      const auto iterator = log_callbacks.find(retired_state);
      if (iterator != log_callbacks.end()) {
        iterator->second.retired = true;
        if (iterator->second.in_flight == 0) {
          retirement_listener = iterator->second.listener;
          log_callbacks.erase(iterator);
        }
      }
    }
  }
  queue_log_retirement(retirement_listener);
  return MLN_STATUS_OK;
}

extern "C" MLN_API void mln_adapter_log_record_destroy(void* record) noexcept {
  destroy_log_record(static_cast<AdapterLogRecordView*>(record));
}

extern "C" MLN_API auto mln_adapter_resource_transform_rewrite_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) noexcept -> mln_status {
  if (user_data == nullptr || url == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto& table =
    *static_cast<const AdapterResourceRewriteRules*>(user_data);
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (
      matches_rule(rule.kind, kind) &&
      url_matches(rule.flags, KnownUrlMatchFlags, rule.url, url)
    ) {
      if (rule.replacement_url == nullptr) {
        return MLN_STATUS_OK;
      }
      return mln_resource_transform_response_set_url(
        out_response, rule.replacement_url, std::strlen(rule.replacement_url)
      );
    }
  }
  return MLN_STATUS_OK;
}

extern "C" MLN_API auto mln_adapter_http_header_transform_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) noexcept -> mln_status {
  if (user_data == nullptr || url == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto& table =
    *static_cast<const AdapterHttpHeaderTransformRules*>(user_data);
  if (table.rules == nullptr && table.count != 0) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (
      !matches_rule(rule.kind, kind) ||
      !url_matches(rule.flags, KnownUrlMatchFlags, rule.url, url)
    ) {
      continue;
    }
    if (rule.headers == nullptr && rule.header_count != 0) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    for (const auto& header : std::span{rule.headers, rule.header_count}) {
      const auto name_size =
        header.name == nullptr ? 0 : std::strlen(header.name);
      const auto value_size =
        header.value == nullptr ? 0 : std::strlen(header.value);
      const auto status = mln_http_header_transform_response_set(
        out_response, header.name, name_size, header.value, value_size
      );
      if (status != MLN_STATUS_OK) {
        return status;
      }
    }
    return MLN_STATUS_OK;
  }
  return MLN_STATUS_OK;
}

extern "C" MLN_API auto mln_adapter_http_header_validate(
  const char* name, const char* value
) noexcept -> mln_status {
  try {
    return mln::core::validate_http_header(
      name, name == nullptr ? 0 : std::strlen(name), value,
      value == nullptr ? 0 : std::strlen(value)
    );
  } catch (...) {
    return MLN_STATUS_NATIVE_ERROR;
  }
}

extern "C" MLN_API auto mln_adapter_resource_provider_rules_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) noexcept -> std::uint32_t {
  if (
    user_data == nullptr || request == nullptr ||
    request->requested_url == nullptr || handle == MLN_HANDLE_NULL
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  const auto& table =
    *static_cast<const AdapterResourceProviderRules*>(user_data);
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (
      matches_rule(rule.kind, request->kind) &&
      url_matches(
        rule.flags, KnownUrlMatchFlags, rule.requested_url,
        request->requested_url
      )
    ) {
      static_cast<void>(mln_resource_request_complete(handle, &rule.response));
      mln_resource_request_release(handle);
      return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
    }
  }
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

extern "C" MLN_API auto mln_adapter_queued_resource_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) noexcept -> std::uint32_t {
  // Each route decides which URL it compares, so route matching handles an
  // absent URL.
  if (user_data == nullptr || request == nullptr || handle == MLN_HANDLE_NULL) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  const auto& provider =
    *static_cast<const AdapterQueuedResourceProvider*>(user_data);
  if (provider.listener == nullptr) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  if (!request_matches_route(
        std::span{provider.routes, provider.route_count}, *request
      )) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  try {
    auto* queued_request = copy_request(*request, handle);
    provider.listener(queued_request);
    return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
  } catch (...) {
    auto response = mln_resource_response{
      .size = sizeof(mln_resource_response),
      .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
      .error_reason = MLN_RESOURCE_ERROR_REASON_OTHER,
      .bytes = nullptr,
      .byte_count = 0,
      .error_message = "resource provider request queue failed",
      .must_revalidate = false,
      .has_modified = false,
      .modified_unix_ms = 0,
      .has_expires = false,
      .expires_unix_ms = 0,
      .etag = nullptr,
      .has_retry_after = false,
      .retry_after_unix_ms = 0,
    };
    static_cast<void>(mln_resource_request_complete(handle, &response));
    mln_resource_request_release(handle);
    return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
  }
}

extern "C" MLN_API void mln_adapter_resource_provider_request_destroy(
  void* request
) noexcept {
  destroy_queued_request(
    static_cast<AdapterQueuedResourceRequestView*>(request)
  );
}

extern "C" MLN_API void mln_adapter_queued_resource_provider_retire(
  mln_adapter_queued_resource_provider* provider
) noexcept {
  if (provider != nullptr && provider->listener != nullptr) {
    provider->listener(nullptr);
  }
}

extern "C" MLN_API void mln_adapter_custom_geometry_callbacks_retire(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile, void* user_data
) noexcept {
  constexpr auto RetirementTile = mln_canonical_tile_id{
    .z = std::numeric_limits<std::uint8_t>::max(),
    .x = 0,
    .y = 0,
  };
  if (fetch_tile != nullptr) {
    fetch_tile(user_data, RetirementTile);
  }
  if (cancel_tile != nullptr) {
    cancel_tile(user_data, RetirementTile);
  }
}

extern "C" MLN_API void mln_adapter_custom_mvt_vector_callbacks_retire(
  mln_custom_mvt_vector_source_tile_callback fetch_tile,
  mln_custom_mvt_vector_source_tile_callback cancel_tile, void* user_data
) noexcept {
  constexpr auto RetirementTile = mln_canonical_tile_id{
    .z = std::numeric_limits<std::uint8_t>::max(),
    .x = 0,
    .y = 0,
  };
  if (fetch_tile != nullptr) {
    fetch_tile(user_data, RetirementTile);
  }
  if (cancel_tile != nullptr) {
    cancel_tile(user_data, RetirementTile);
  }
}
