#define MLN_BUILDING_C

// Dart exposes native callbacks through two different mechanisms. Synchronous
// callbacks have strict thread and isolate limits, while listener callbacks may
// be invoked from arbitrary native threads but must return void and deliver
// work asynchronously to the owning isolate. MapLibre callback contracts need
// more: logging and resource providers return immediate decisions, and borrowed
// request payloads expire when the C callback returns.
//
// This shim handles the native-thread part of those contracts. It copies
// borrowed payloads into small native-owned records, applies native-owned
// routing rules when a result is needed immediately, and invokes only void Dart
// listener functions for isolate delivery. Dart user callbacks therefore run on
// their owning isolate, not on MapLibre worker, network, logging, or render
// threads. The matching private C layouts live in dart_shim.h so ffigen can
// generate the Dart struct declarations from one source of truth.

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
#include <unordered_map>
#include <vector>

#include "dart_shim.h"

#include "maplibre_native_c.h"

namespace {

constexpr std::uint32_t DartResourceKindWildcard =
  std::numeric_limits<std::uint32_t>::max();

using DartResourceRewriteRule = mln_dart_resource_rewrite_rule;
using DartResourceRewriteRules = mln_dart_resource_rewrite_rules;
using DartResourceProviderRule = mln_dart_resource_provider_rule;
using DartResourceProviderRules = mln_dart_resource_provider_rules;
using DartQueuedResourceProviderRoute = mln_dart_queued_resource_provider_route;
using DartQueuedResourceProvider = mln_dart_queued_resource_provider;
using DartQueuedResourceRequestView = mln_dart_queued_resource_request;

struct DartQueuedResourceRequest {
  DartQueuedResourceRequestView view{};
  std::string url;
  std::string prior_etag;
  std::vector<std::uint8_t> prior_data;
};

using DartLogCallbackState = mln_dart_log_callback_state;
using DartLogRecordView = mln_dart_log_record;

struct DartLogRecord {
  DartLogRecordView view{};
  std::string message;
};

struct DartLogCallbackEntry {
  mln_dart_log_record_listener listener = nullptr;
  std::uint32_t consume = 0;
  std::size_t in_flight = 0;
  bool retired = false;
};

struct DartHandleLeakToken {
  std::string type_name;
};

std::mutex resource_request_tokens_mutex;
std::condition_variable resource_request_tokens_changed;
std::unordered_map<std::uint64_t, mln_resource_request_handle*>
  resource_request_tokens;
std::uint64_t next_resource_request_token = 1;
std::mutex dart_log_setter_mutex;
std::mutex dart_log_state_mutex;
std::unordered_map<void*, DartLogCallbackEntry> dart_log_callbacks;
void* active_dart_log_callback = nullptr;

auto matches_rule(std::uint32_t rule_kind, std::uint32_t request_kind) -> bool {
  return rule_kind == DartResourceKindWildcard || rule_kind == request_kind;
}

auto string_equals(const char* left, const char* right) -> bool {
  if (left == nullptr || right == nullptr) {
    return false;
  }
  return std::strcmp(left, right) == 0;
}

auto request_matches_route(
  std::span<const DartQueuedResourceProviderRoute> routes,
  const mln_resource_request& request
) -> bool {
  return std::ranges::any_of(routes, [&request](const auto& route) -> bool {
    return matches_rule(route.kind, request.kind) &&
           string_equals(route.url, request.url);
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
  const mln_resource_request& request, mln_resource_request_handle* handle
) -> DartQueuedResourceRequestView* {
  auto copy = std::make_unique<DartQueuedResourceRequest>();
  copy->url = request.url == nullptr ? std::string{} : std::string{request.url};
  copy->prior_etag = request.prior_etag == nullptr
                       ? std::string{}
                       : std::string{request.prior_etag};
  copy->prior_data = copy_prior_data(request);
  copy->view = DartQueuedResourceRequestView{
    .owner = copy.get(),
    .handle = handle,
    .url = copy->url.c_str(),
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

void destroy_queued_request(DartQueuedResourceRequestView* request) noexcept {
  if (request == nullptr) {
    return;
  }
  auto* owner = static_cast<DartQueuedResourceRequest*>(request->owner);
  static_cast<void>(std::unique_ptr<DartQueuedResourceRequest>{owner});
}

auto copy_log_record(
  std::uint32_t severity, std::uint32_t event, std::int64_t code,
  const char* message, bool retire_callback = false
) -> DartLogRecordView* {
  auto copy = std::make_unique<DartLogRecord>();
  copy->message = message == nullptr ? std::string{} : std::string{message};
  copy->view = DartLogRecordView{
    .owner = copy.get(),
    .retire_callback = retire_callback,
    .severity = severity,
    .event = event,
    .code = code,
    .message = copy->message.c_str(),
  };
  return &copy.release()->view;
}

void destroy_log_record(DartLogRecordView* record) noexcept {
  if (record == nullptr) {
    return;
  }
  auto* owner = static_cast<DartLogRecord*>(record->owner);
  static_cast<void>(std::unique_ptr<DartLogRecord>{owner});
}

void queue_log_retirement(mln_dart_log_record_listener listener) noexcept {
  if (listener == nullptr) {
    return;
  }
  try {
    listener(nullptr);
  } catch (...) {
    // The callback has already been removed from native dispatch. Listener
    // delivery is notification-only at this boundary.
  }
}

void destroy_handle_leak_token(void* token) noexcept {
  static_cast<void>(std::unique_ptr<DartHandleLeakToken>{
    static_cast<DartHandleLeakToken*>(token),
  });
}

}  // namespace

extern "C" MLN_API auto mln_dart_handle_leak_token_create(
  const char* type_name, void* handle
) noexcept -> void* {
  try {
    auto token = std::make_unique<DartHandleLeakToken>();
    static_cast<void>(handle);
    token->type_name = type_name == nullptr ? std::string{} : type_name;
    return token.release();
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API void mln_dart_handle_leak_token_destroy(
  void* token
) noexcept {
  destroy_handle_leak_token(token);
}

extern "C" MLN_API void mln_dart_handle_leak_report(void* token) noexcept {
  auto* leak = static_cast<DartHandleLeakToken*>(token);
  if (leak != nullptr) {
    static_cast<void>(std::fputs("maplibre_native_ffi: leaked ", stderr));
    static_cast<void>(std::fputs(leak->type_name.c_str(), stderr));
    static_cast<void>(std::fputs(
      " native handle; call close() on the owner isolate before releasing the "
      "Dart object\n",
      stderr
    ));
  }
  destroy_handle_leak_token(token);
}

extern "C" MLN_API auto mln_dart_log_callback(
  void* user_data, std::uint32_t severity, std::uint32_t event,
  std::int64_t code, const char* message
) noexcept -> std::uint32_t {
  if (user_data == nullptr) {
    return 0;
  }
  mln_dart_log_record_listener listener = nullptr;
  std::uint32_t consume = 0;
  {
    const auto lock = std::scoped_lock{dart_log_state_mutex};
    const auto iterator = dart_log_callbacks.find(user_data);
    if (iterator == dart_log_callbacks.end() || iterator->second.retired) {
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
      // Logging callbacks are notification-only at the Dart boundary.
    }
  }
  mln_dart_log_record_listener retirement_listener = nullptr;
  {
    const auto lock = std::scoped_lock{dart_log_state_mutex};
    const auto iterator = dart_log_callbacks.find(user_data);
    if (iterator != dart_log_callbacks.end()) {
      --iterator->second.in_flight;
      if (iterator->second.retired && iterator->second.in_flight == 0) {
        retirement_listener = iterator->second.listener;
        dart_log_callbacks.erase(iterator);
      }
    }
  }
  queue_log_retirement(retirement_listener);
  return consume;
}

extern "C" MLN_API auto mln_dart_log_set_callback(
  mln_dart_log_callback_state* state
) noexcept -> mln_status {
  const auto setter_lock = std::scoped_lock{dart_log_setter_mutex};
  if (state != nullptr) {
    const auto state_lock = std::scoped_lock{dart_log_state_mutex};
    dart_log_callbacks[state] = DartLogCallbackEntry{
      .listener = state->listener,
      .consume = state->consume,
    };
  }
  const auto status = state == nullptr
                        ? mln_log_clear_callback()
                        : mln_log_set_callback(mln_dart_log_callback, state);
  if (status != MLN_STATUS_OK) {
    if (state != nullptr) {
      const auto state_lock = std::scoped_lock{dart_log_state_mutex};
      dart_log_callbacks.erase(state);
    }
    return status;
  }
  mln_dart_log_record_listener retirement_listener = nullptr;
  {
    const auto state_lock = std::scoped_lock{dart_log_state_mutex};
    auto* retired_state = active_dart_log_callback;
    active_dart_log_callback = state;
    if (retired_state != nullptr && retired_state != state) {
      const auto iterator = dart_log_callbacks.find(retired_state);
      if (iterator != dart_log_callbacks.end()) {
        iterator->second.retired = true;
        if (iterator->second.in_flight == 0) {
          retirement_listener = iterator->second.listener;
          dart_log_callbacks.erase(iterator);
        }
      }
    }
  }
  queue_log_retirement(retirement_listener);
  return MLN_STATUS_OK;
}

extern "C" MLN_API void mln_dart_log_record_destroy(void* record) noexcept {
  destroy_log_record(static_cast<DartLogRecordView*>(record));
}

extern "C" MLN_API auto mln_dart_resource_transform_rewrite_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) noexcept -> mln_status {
  if (user_data == nullptr || url == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto& table = *static_cast<const DartResourceRewriteRules*>(user_data);
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (matches_rule(rule.kind, kind) && string_equals(rule.url, url)) {
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

extern "C" MLN_API auto mln_dart_resource_provider_rules_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle* handle
) noexcept -> std::uint32_t {
  if (
    user_data == nullptr || request == nullptr || request->url == nullptr ||
    handle == nullptr
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  const auto& table = *static_cast<const DartResourceProviderRules*>(user_data);
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (
      matches_rule(rule.kind, request->kind) &&
      string_equals(rule.url, request->url)
    ) {
      static_cast<void>(mln_resource_request_complete(handle, &rule.response));
      mln_resource_request_release(handle);
      return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
    }
  }
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

extern "C" MLN_API auto mln_dart_queued_resource_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle* handle
) noexcept -> std::uint32_t {
  if (
    user_data == nullptr || request == nullptr || request->url == nullptr ||
    handle == nullptr
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  const auto& provider =
    *static_cast<const DartQueuedResourceProvider*>(user_data);
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

extern "C" MLN_API void mln_dart_resource_provider_request_destroy(
  void* request
) noexcept {
  destroy_queued_request(static_cast<DartQueuedResourceRequestView*>(request));
}

extern "C" MLN_API void mln_dart_queued_resource_provider_retire(
  mln_dart_queued_resource_provider* provider
) noexcept {
  if (provider != nullptr && provider->listener != nullptr) {
    provider->listener(nullptr);
  }
}

extern "C" MLN_API void mln_dart_custom_geometry_callbacks_retire(
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

extern "C" MLN_API auto mln_dart_resource_request_token_create(
  mln_resource_request_handle* handle
) noexcept -> std::uint64_t {
  if (handle == nullptr) {
    return 0;
  }
  try {
    const auto lock = std::scoped_lock{resource_request_tokens_mutex};
    auto token = next_resource_request_token++;
    while (token == 0 || resource_request_tokens.contains(token)) {
      token = next_resource_request_token++;
    }
    resource_request_tokens.emplace(token, handle);
    return token;
  } catch (...) {
    return 0;
  }
}

extern "C" MLN_API auto mln_dart_resource_request_token_cancelled(
  std::uint64_t token, bool* out_cancelled
) noexcept -> mln_status {
  if (token == 0 || out_cancelled == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto lock = std::scoped_lock{resource_request_tokens_mutex};
  const auto iterator = resource_request_tokens.find(token);
  if (iterator == resource_request_tokens.end()) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return mln_resource_request_cancelled(iterator->second, out_cancelled);
}

extern "C" MLN_API auto mln_dart_resource_request_token_complete(
  std::uint64_t token, const mln_resource_response* response
) noexcept -> mln_status {
  if (token == 0 || response == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto lock = std::scoped_lock{resource_request_tokens_mutex};
  const auto iterator = resource_request_tokens.find(token);
  if (iterator == resource_request_tokens.end()) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* handle = iterator->second;
  const auto status = mln_resource_request_complete(handle, response);
  mln_resource_request_release(handle);
  resource_request_tokens.erase(iterator);
  resource_request_tokens_changed.notify_all();
  return status;
}

extern "C" MLN_API auto mln_dart_resource_request_token_release(
  std::uint64_t token
) noexcept -> mln_status {
  if (token == 0) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto lock = std::scoped_lock{resource_request_tokens_mutex};
  const auto iterator = resource_request_tokens.find(token);
  if (iterator == resource_request_tokens.end()) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  mln_resource_request_release(iterator->second);
  resource_request_tokens.erase(iterator);
  resource_request_tokens_changed.notify_all();
  return MLN_STATUS_OK;
}

extern "C" MLN_API auto mln_dart_resource_request_token_wait(
  std::uint64_t token
) noexcept -> mln_status {
  if (token == 0) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto lock = std::unique_lock{resource_request_tokens_mutex};
  resource_request_tokens_changed.wait(lock, [token] {
    return !resource_request_tokens.contains(token);
  });
  return MLN_STATUS_OK;
}

extern "C" MLN_API void mln_dart_test_invoke_custom_geometry_tile_callback(
  mln_custom_geometry_source_tile_callback callback, void* user_data,
  mln_canonical_tile_id tile_id
) noexcept {
  if (callback != nullptr) {
    callback(user_data, tile_id);
  }
}
