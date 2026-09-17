#define MLN_BUILDING_C

// Adapts synchronous MapLibre callback contracts to hosts that can only receive
// callbacks asynchronously through void listener functions.
//
// Native callbacks enqueue on MapLibre threads; hosts drain and close queues
// from their own execution contexts.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "maplibre_native_c/callback_adapter.h"

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"
#include "wake/wake.hpp"

namespace mln::core {

struct AdapterQueuedResourceRequest {
  mln_adapter_queued_resource_request view{};
  std::string requested_url;
  std::string resolved_url;
  std::string prior_etag;
  std::vector<std::uint8_t> prior_data;
};

struct AdapterLogRecord {
  mln_adapter_log_record view{};
  std::string message;
};

struct AdapterCompletionRecord {
  mln_adapter_completion_record view{};
  std::vector<std::byte> flat;
  std::deque<std::string> strings;
  std::vector<mln_buffer_view> views;
  std::vector<mln_offline_region_info> offline_regions;
  mln_style_source_result style_source{};
  mln_style_source_tile_urls_result style_source_tile_urls{};
  mln_style_layer_result style_layer{};
  mln_style_image_result style_image{};
  mln_style_image_stretches_result image_stretches{};
  std::vector<mln_image_stretch> stretch_x;
  std::vector<mln_image_stretch> stretch_y;
  std::vector<mln_queried_feature> queried_features;
  mln_texture_readback_result texture_readback{};
};

struct AdapterResourceRequestQueueObject {
  std::mutex mutex;
  std::mutex drain_mutex;
  std::deque<std::unique_ptr<AdapterQueuedResourceRequest>> records;
  std::shared_ptr<Wake> wake;
  bool wake_pending = false;
  bool closed = false;

  auto close() noexcept -> void {
    auto discarded = decltype(records){};
    auto detached_wake = std::shared_ptr<Wake>{};
    {
      const std::scoped_lock lock(mutex);
      if (closed) {
        return;
      }
      closed = true;
      discarded.swap(records);
      detached_wake = std::move(wake);
    }
    detached_wake.reset();
    for (const auto& record : discarded) {
      mln_resource_request_release(record->view.handle);
    }
  }

  ~AdapterResourceRequestQueueObject() { close(); }
};

struct AdapterLogQueueObject {
  std::mutex mutex;
  std::mutex drain_mutex;
  std::deque<std::unique_ptr<AdapterLogRecord>> records;
  std::shared_ptr<Wake> wake;
  bool wake_pending = false;
  bool closed = false;

  auto close() noexcept -> void {
    auto discarded = decltype(records){};
    auto detached_wake = std::shared_ptr<Wake>{};
    {
      const std::scoped_lock lock(mutex);
      if (closed) {
        return;
      }
      closed = true;
      discarded.swap(records);
      detached_wake = std::move(wake);
    }
    detached_wake.reset();
  }

  ~AdapterLogQueueObject() { close(); }
};

template <>
struct HandleTraits<AdapterResourceRequestQueueObject> {
  static constexpr auto kind = HandleKind::AdapterResourceRequestQueue;
  static constexpr auto leasable = true;
};

template <>
struct HandleTraits<AdapterLogQueueObject> {
  static constexpr auto kind = HandleKind::AdapterLogQueue;
  static constexpr auto leasable = true;
};

}  // namespace mln::core

namespace {

using AdapterResourceRewriteRules = mln_adapter_resource_rewrite_rules;
using AdapterHttpHeaderTransformRules = mln_adapter_http_header_transform_rules;
using AdapterResourceProviderRules = mln_adapter_resource_provider_rules;
using AdapterQueuedResourceProviderRoute =
  mln_adapter_queued_resource_provider_route;
using AdapterQueuedResourceProvider = mln_adapter_queued_resource_provider;
using AdapterQueuedResourceRequest = mln::core::AdapterQueuedResourceRequest;
using AdapterQueuedResourceRequestView = mln_adapter_queued_resource_request;
using AdapterLogCallbackState = mln_adapter_log_callback_state;
using AdapterLogRecord = mln::core::AdapterLogRecord;
using AdapterCompletionRecord = mln::core::AdapterCompletionRecord;

struct AdapterHandleLeakToken {
  std::string type_name;
  std::uint64_t handle = 0;
};
using AdapterLogRecordView = mln_adapter_log_record;

std::mutex log_setter_mutex;

struct AdapterCompletionState {
  std::uint32_t copy_kind = MLN_ADAPTER_COMPLETION_COPY_FLAT;
  std::size_t element_size = 0;
  mln_adapter_completion_listener listener = nullptr;
  void* user_data = nullptr;
};

auto copy_bytes(
  AdapterCompletionRecord& record, const void* data, std::size_t size
) -> mln_buffer_view {
  if (size == 0) return {};
  if (data == nullptr) throw std::invalid_argument{"completion view is null"};
  const auto* first = static_cast<const char*>(data);
  record.strings.emplace_back(first, size);
  const auto& copy = record.strings.back();
  return {.data = copy.data(), .size = copy.size()};
}

auto copy_c_string(AdapterCompletionRecord& record, const char* value) -> const
  char* {
  if (value == nullptr) return nullptr;
  record.strings.emplace_back(value);
  return record.strings.back().c_str();
}

auto copy_view(AdapterCompletionRecord& record, mln_buffer_view value)
  -> mln_buffer_view {
  return copy_bytes(record, value.data, value.size);
}

auto copy_offline_region(
  AdapterCompletionRecord& record, mln_offline_region_info value
) -> mln_offline_region_info {
  switch (value.definition.type) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID:
      value.definition.data.tile_pyramid.style_url =
        copy_c_string(record, value.definition.data.tile_pyramid.style_url);
      break;
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY:
      value.definition.data.geometry.style_url =
        copy_c_string(record, value.definition.data.geometry.style_url);
      value.definition.data.geometry.geometry =
        copy_view(record, value.definition.data.geometry.geometry);
      break;
    default:
      throw std::invalid_argument{"completion has an unknown offline region"};
  }
  const auto metadata = copy_bytes(record, value.metadata, value.metadata_size);
  value.metadata = static_cast<const std::uint8_t*>(metadata.data);
  value.metadata_size = metadata.size;
  return value;
}

auto copy_completion_value(
  AdapterCompletionRecord& record, const mln_completion_result& result,
  const AdapterCompletionState& state
) -> void {
  if (result.value_count == 0) return;
  if (result.value == nullptr) {
    throw std::invalid_argument{"completion value is null"};
  }
  switch (state.copy_kind) {
    case MLN_ADAPTER_COMPLETION_COPY_FLAT: {
      if (
        state.element_size == 0 ||
        result.value_count >
          std::numeric_limits<std::size_t>::max() / state.element_size
      ) {
        throw std::invalid_argument{"completion element size is invalid"};
      }
      const auto size = result.value_count * state.element_size;
      record.flat.resize(size);
      std::memcpy(record.flat.data(), result.value, size);
      record.view.result.value = record.flat.data();
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS: {
      const auto values = std::span{
        static_cast<const mln_buffer_view*>(result.value), result.value_count
      };
      record.views.reserve(values.size());
      for (const auto& value : values) {
        record.views.push_back(copy_view(record, value));
      }
      record.view.result.value = record.views.data();
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGIONS: {
      const auto values = std::span{
        static_cast<const mln_offline_region_info*>(result.value),
        result.value_count
      };
      record.offline_regions.reserve(values.size());
      for (const auto& value : values) {
        record.offline_regions.push_back(copy_offline_region(record, value));
      }
      record.view.result.value = record.offline_regions.data();
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE: {
      if (result.value_count != 1)
        throw std::invalid_argument{"invalid source result"};
      record.style_source =
        *static_cast<const mln_style_source_result*>(result.value);
      record.style_source.attribution =
        copy_view(record, record.style_source.attribution);
      record.style_source.url = copy_view(record, record.style_source.url);
      const auto urls = std::span{
        record.style_source.tile_urls, record.style_source.tile_url_count
      };
      record.views.reserve(urls.size());
      for (const auto url : urls)
        record.views.push_back(copy_view(record, url));
      record.style_source.tile_urls = record.views.data();
      record.view.result.value = &record.style_source;
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_TILE_URLS: {
      if (result.value_count != 1)
        throw std::invalid_argument{"invalid tile URL result"};
      record.style_source_tile_urls =
        *static_cast<const mln_style_source_tile_urls_result*>(result.value);
      const auto urls = std::span{
        record.style_source_tile_urls.tile_urls,
        record.style_source_tile_urls.tile_url_count
      };
      record.views.reserve(urls.size());
      for (const auto url : urls)
        record.views.push_back(copy_view(record, url));
      record.style_source_tile_urls.tile_urls = record.views.data();
      record.view.result.value = &record.style_source_tile_urls;
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER: {
      if (result.value_count != 1)
        throw std::invalid_argument{"invalid layer result"};
      record.style_layer =
        *static_cast<const mln_style_layer_result*>(result.value);
      record.style_layer.info.type =
        copy_view(record, record.style_layer.info.type);
      record.style_layer.source_id =
        copy_view(record, record.style_layer.source_id);
      record.style_layer.source_layer =
        copy_view(record, record.style_layer.source_layer);
      record.view.result.value = &record.style_layer;
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE: {
      if (result.value_count != 1)
        throw std::invalid_argument{"invalid image result"};
      record.style_image =
        *static_cast<const mln_style_image_result*>(result.value);
      record.style_image.pixels = copy_view(record, record.style_image.pixels);
      if (record.style_image.stretch_x_count != 0) {
        record.stretch_x.assign(
          record.style_image.stretch_x,
          record.style_image.stretch_x + record.style_image.stretch_x_count
        );
      }
      if (record.style_image.stretch_y_count != 0) {
        record.stretch_y.assign(
          record.style_image.stretch_y,
          record.style_image.stretch_y + record.style_image.stretch_y_count
        );
      }
      record.style_image.stretch_x = record.stretch_x.data();
      record.style_image.stretch_y = record.stretch_y.data();
      record.view.result.value = &record.style_image;
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_STRETCHES: {
      if (result.value_count != 1)
        throw std::invalid_argument{"invalid stretch result"};
      record.image_stretches =
        *static_cast<const mln_style_image_stretches_result*>(result.value);
      if (record.image_stretches.stretch_x_count != 0) {
        record.stretch_x.assign(
          record.image_stretches.stretch_x,
          record.image_stretches.stretch_x +
            record.image_stretches.stretch_x_count
        );
      }
      if (record.image_stretches.stretch_y_count != 0) {
        record.stretch_y.assign(
          record.image_stretches.stretch_y,
          record.image_stretches.stretch_y +
            record.image_stretches.stretch_y_count
        );
      }
      record.image_stretches.stretch_x = record.stretch_x.data();
      record.image_stretches.stretch_y = record.stretch_y.data();
      record.view.result.value = &record.image_stretches;
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_QUERIED_FEATURES: {
      const auto values = std::span{
        static_cast<const mln_queried_feature*>(result.value),
        result.value_count
      };
      record.queried_features.reserve(values.size());
      for (auto value : values) {
        value.feature = copy_view(record, value.feature);
        value.source_id = copy_view(record, value.source_id);
        value.source_layer_id = copy_view(record, value.source_layer_id);
        value.state = copy_view(record, value.state);
        record.queried_features.push_back(value);
      }
      record.view.result.value = record.queried_features.data();
      break;
    }
    case MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK: {
      if (result.value_count != 1)
        throw std::invalid_argument{"invalid readback result"};
      record.texture_readback =
        *static_cast<const mln_texture_readback_result*>(result.value);
      record.texture_readback.data =
        copy_view(record, record.texture_readback.data);
      record.view.result.value = &record.texture_readback;
      break;
    }
    default:
      throw std::invalid_argument{"unknown completion copy kind"};
  }
}

auto adapter_completion_callback(
  void* user_data, const mln_completion_result* result
) noexcept -> void {
  const auto* state = static_cast<const AdapterCompletionState*>(user_data);
  if (state == nullptr || result == nullptr || state->listener == nullptr)
    return;
  auto record =
    std::unique_ptr<AdapterCompletionRecord>{new (std::nothrow)
                                               AdapterCompletionRecord{}};
  if (!record) {
    state->listener(state->user_data, nullptr);
    return;
  }
  record->view.owner = record.get();
  record->view.result = *result;
  record->view.result.diagnostic = {};
  record->view.result.value = nullptr;
  try {
    record->view.result.diagnostic = copy_view(*record, result->diagnostic);
    if (result->status == MLN_STATUS_OK) {
      copy_completion_value(*record, *result, *state);
    } else {
      record->view.result.value_count = 0;
    }
  } catch (...) {
    state->listener(state->user_data, nullptr);
    return;
  }
  auto* delivered = &record.release()->view;
  state->listener(state->user_data, delivered);
}

auto adapter_completion_release(void* user_data) noexcept -> void {
  delete static_cast<AdapterCompletionState*>(user_data);
}

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
) -> std::unique_ptr<AdapterQueuedResourceRequest> {
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
  return copy;
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
  const char* message
) -> std::unique_ptr<AdapterLogRecord> {
  auto copy = std::make_unique<AdapterLogRecord>();
  copy->message = message == nullptr ? std::string{} : std::string{message};
  copy->view = AdapterLogRecordView{
    .owner = copy.get(),
    .severity = severity,
    .event = event,
    .code = code,
    .message = copy->message.c_str(),
  };
  return copy;
}

void destroy_log_record(AdapterLogRecordView* record) noexcept {
  if (record == nullptr) {
    return;
  }
  auto* owner = static_cast<AdapterLogRecord*>(record->owner);
  static_cast<void>(std::unique_ptr<AdapterLogRecord>{owner});
}
auto lease_resource_queue(mln_adapter_resource_request_queue queue)
  -> std::shared_ptr<mln::core::AdapterResourceRequestQueueObject> {
  return mln::core::handle_table<mln::core::AdapterResourceRequestQueueObject>()
    .lease(queue);
}

auto lease_log_queue(mln_adapter_log_queue queue)
  -> std::shared_ptr<mln::core::AdapterLogQueueObject> {
  return mln::core::handle_table<mln::core::AdapterLogQueueObject>().lease(
    queue
  );
}

auto enqueue_request(
  const std::shared_ptr<mln::core::AdapterResourceRequestQueueObject>& queue,
  std::unique_ptr<AdapterQueuedResourceRequest> request
) -> bool {
  auto wake = std::shared_ptr<mln::core::Wake>{};
  auto should_wake = false;
  {
    const std::scoped_lock lock(queue->mutex);
    if (queue->closed) {
      return false;
    }
    should_wake = queue->records.empty();
    queue->records.push_back(std::move(request));
    queue->wake_pending = true;
    wake = queue->wake;
  }
  if (should_wake) wake->notify();
  return true;
}

auto enqueue_log(
  const std::shared_ptr<mln::core::AdapterLogQueueObject>& queue,
  std::unique_ptr<AdapterLogRecord> record
) -> bool {
  auto wake = std::shared_ptr<mln::core::Wake>{};
  auto should_wake = false;
  {
    const std::scoped_lock lock(queue->mutex);
    if (queue->closed) {
      return false;
    }
    should_wake = queue->records.empty();
    queue->records.push_back(std::move(record));
    queue->wake_pending = true;
    wake = queue->wake;
  }
  if (should_wake) wake->notify();
  return true;
}

void destroy_handle_leak_token(void* token) noexcept {
  static_cast<void>(std::unique_ptr<AdapterHandleLeakToken>{
    static_cast<AdapterHandleLeakToken*>(token),
  });
}

}  // namespace

extern "C" MLN_API auto mln_adapter_completion_create(
  std::uint32_t copy_kind, std::size_t element_size,
  mln_adapter_completion_listener listener, void* user_data,
  mln_completion* out_completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (
      out_completion == nullptr || out_completion->size != 0 ||
      out_completion->callback != nullptr ||
      out_completion->user_data != nullptr ||
      out_completion->release_user_data != nullptr || listener == nullptr ||
      copy_kind > MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_TILE_URLS
    ) {
      mln::core::set_thread_error("completion adapter arguments are invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto state = std::make_unique<AdapterCompletionState>();
    state->copy_kind = copy_kind;
    state->element_size = element_size;
    state->listener = listener;
    state->user_data = user_data;
    *out_completion = mln_completion{
      .size = sizeof(mln_completion),
      .callback = adapter_completion_callback,
      .user_data = state.get(),
      .release_user_data = adapter_completion_release,
    };
    static_cast<void>(state.release());
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API void mln_adapter_completion_reject(
  mln_completion* completion
) noexcept {
  if (
    completion == nullptr ||
    completion->callback != adapter_completion_callback ||
    completion->release_user_data != adapter_completion_release
  ) {
    return;
  }
  adapter_completion_release(completion->user_data);
  *completion = {};
}

extern "C" MLN_API void mln_adapter_completion_record_destroy(
  mln_adapter_completion_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  delete static_cast<AdapterCompletionRecord*>(record->owner);
}

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

extern "C" MLN_API auto mln_adapter_resource_request_queue_create(
  const mln_wake* wake, mln_adapter_resource_request_queue* out_queue
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_queue == nullptr || *out_queue != MLN_HANDLE_NULL) {
      mln::core::set_thread_error("out_queue must point to the null handle");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto wake_status = mln::core::validate_wake(wake);
    if (wake_status != MLN_STATUS_OK) return wake_status;
    auto owned =
      std::make_shared<mln::core::AdapterResourceRequestQueueObject>();
    owned->wake = std::make_shared<mln::core::Wake>(*wake);
    const auto handle =
      mln::core::handle_table<mln::core::AdapterResourceRequestQueueObject>()
        .insert(owned);
    owned->wake->accept();
    *out_queue = handle;
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_resource_request_queue_acquire(
  mln_adapter_resource_request_queue queue,
  mln_adapter_queued_resource_request** out_request
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_request == nullptr || *out_request != nullptr) {
      mln::core::set_thread_error("out_request must point to null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto live = lease_resource_queue(queue);
    if (live == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto drain_lock = std::unique_lock{live->drain_mutex, std::try_to_lock};
    if (!drain_lock.owns_lock()) {
      mln::core::set_thread_error(
        "resource request queue already has an active drain"
      );
      return MLN_STATUS_INVALID_STATE;
    }
    const auto queue_lock = std::scoped_lock{live->mutex};
    if (live->closed) {
      mln::core::set_thread_error("resource request queue is closed");
      return MLN_STATUS_INVALID_STATE;
    }
    if (live->records.empty()) {
      live->wake_pending = false;
      return MLN_STATUS_OK;
    }
    auto record = std::move(live->records.front());
    live->records.pop_front();
    *out_request = &record.release()->view;
    if (live->records.empty()) {
      live->wake_pending = false;
    }
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API void mln_adapter_resource_request_queue_close(
  mln_adapter_resource_request_queue queue
) noexcept {
  const auto removed =
    mln::core::handle_table<mln::core::AdapterResourceRequestQueueObject>()
      .remove(queue);
  if (removed != nullptr) {
    removed->close();
  }
}

extern "C" MLN_API auto mln_adapter_log_queue_create(
  const mln_wake* wake, mln_adapter_log_queue* out_queue
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_queue == nullptr || *out_queue != MLN_HANDLE_NULL) {
      mln::core::set_thread_error("out_queue must point to the null handle");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto wake_status = mln::core::validate_wake(wake);
    if (wake_status != MLN_STATUS_OK) return wake_status;
    auto owned = std::make_shared<mln::core::AdapterLogQueueObject>();
    owned->wake = std::make_shared<mln::core::Wake>(*wake);
    const auto handle =
      mln::core::handle_table<mln::core::AdapterLogQueueObject>().insert(owned);
    owned->wake->accept();
    *out_queue = handle;
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_log_queue_acquire(
  mln_adapter_log_queue queue, mln_adapter_log_record** out_record
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_record == nullptr || *out_record != nullptr) {
      mln::core::set_thread_error("out_record must point to null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto live = lease_log_queue(queue);
    if (live == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto drain_lock = std::unique_lock{live->drain_mutex, std::try_to_lock};
    if (!drain_lock.owns_lock()) {
      mln::core::set_thread_error("log queue already has an active drain");
      return MLN_STATUS_INVALID_STATE;
    }
    const auto queue_lock = std::scoped_lock{live->mutex};
    if (live->closed) {
      mln::core::set_thread_error("log queue is closed");
      return MLN_STATUS_INVALID_STATE;
    }
    if (live->records.empty()) {
      live->wake_pending = false;
      return MLN_STATUS_OK;
    }
    auto record = std::move(live->records.front());
    live->records.pop_front();
    *out_record = &record.release()->view;
    if (live->records.empty()) {
      live->wake_pending = false;
    }
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API void mln_adapter_log_queue_close(
  mln_adapter_log_queue queue
) noexcept {
  const auto removed =
    mln::core::handle_table<mln::core::AdapterLogQueueObject>().remove(queue);
  if (removed != nullptr) {
    removed->close();
  }
}

extern "C" MLN_API auto mln_adapter_log_callback(
  void* user_data, std::uint32_t severity, std::uint32_t event,
  std::int64_t code, const char* message
) noexcept -> std::uint32_t {
  if (user_data == nullptr) {
    return 0;
  }
  const auto& state = *static_cast<const AdapterLogCallbackState*>(user_data);
  const auto queue = lease_log_queue(state.queue);
  if (queue == nullptr) {
    return 0;
  }
  try {
    static_cast<void>(
      enqueue_log(queue, copy_log_record(severity, event, code, message))
    );
  } catch (...) {
    // Logging cannot report allocation failure through its callback contract.
  }
  return state.consume;
}

namespace {

auto release_adapter_log_callback_state(void* user_data) noexcept -> void {
  auto* state = static_cast<mln_adapter_log_callback_state*>(user_data);
  if (state != nullptr && state->release_user_data != nullptr) {
    const auto release = state->release_user_data;
    const auto context = state->release_context;
    release(context);
  }
}

}  // namespace

extern "C" MLN_API auto mln_adapter_log_set_callback(
  mln_adapter_log_callback_state* state
) noexcept -> mln_status {
  const auto setter_lock = std::scoped_lock{log_setter_mutex};
  if (state != nullptr && lease_log_queue(state->queue) == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return state == nullptr ? mln_log_clear_callback()
                          : mln_log_set_callback(
                              mln_adapter_log_callback, state,
                              state->release_user_data == nullptr
                                ? nullptr
                                : release_adapter_log_callback_state
                            );
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
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::validate_http_header(
      name, name == nullptr ? 0 : std::strlen(name), value,
      value == nullptr ? 0 : std::strlen(value)
    );
  });
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
  if (!request_matches_route(
        std::span{provider.routes, provider.route_count}, *request
      )) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  try {
    const auto queue = lease_resource_queue(provider.queue);
    if (queue == nullptr) {
      throw std::runtime_error{"resource request queue is unavailable"};
    }
    if (!enqueue_request(queue, copy_request(*request, handle))) {
      throw std::runtime_error{"resource request queue is closed"};
    }
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
