#define MLN_BUILDING_C

// Adapts synchronous MapLibre callback contracts to hosts that can only receive
// callbacks asynchronously through void listener functions.
//
// Native callbacks enqueue on MapLibre threads; hosts drain and close queues
// from their own execution contexts.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "maplibre_native_c/callback_adapter.h"

#include "c_api/boundary.hpp"
#include "c_api/callback_capture.hpp"
#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "resources/custom_resource_provider.hpp"
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

struct AdapterResourceRequestQueueObject {
  std::mutex mutex;
  std::mutex drain_mutex;
  std::deque<std::unique_ptr<AdapterQueuedResourceRequest>> records;
  std::shared_ptr<Wake> wake;
  bool wake_pending = false;
  bool closed = false;

  auto close() noexcept -> void {
    auto detached_wake = std::shared_ptr<Wake>{};
    {
      const std::scoped_lock lock(mutex);
      if (closed) {
        return;
      }
      closed = true;
      detached_wake = std::move(wake);
    }
    detached_wake.reset();
    // Closing excludes producers and drainers before releasing host-owned
    // requests. Constructing an empty deque can allocate on Windows.
    for (const auto& record : records) {
      mln_resource_request_release(record->view.handle);
    }
    records.clear();
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
    auto detached_wake = std::shared_ptr<Wake>{};
    {
      const std::scoped_lock lock(mutex);
      if (closed) {
        return;
      }
      closed = true;
      detached_wake = std::move(wake);
    }
    detached_wake.reset();
    records.clear();
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
using AdapterCompletionRecord = mln::capture::Record;

// Dart native API major version 2 fixes this integer-message prefix. Only
// kInt64 is used, so the larger union alternatives are never accessed.
struct DartIntegerMessage {
  std::int32_t type = 3;
  union {
    std::int64_t value = 0;
    struct {
      std::intptr_t length;
      DartIntegerMessage** values;
    } array;
    struct {
      std::intptr_t pointer;
      std::intptr_t size;
      void (*finalize)(void*, void*);
    } native_pointer;
    std::uintptr_t reserved[5];
  };
};
struct DartWake {
  using Post = bool (*)(std::int64_t, DartIntegerMessage*);
  Post post;
  std::int64_t port;

  void notify(std::int64_t value) const noexcept {
    auto message = DartIntegerMessage{3, {value}};
    static_cast<void>(post(port, &message));
  }
};

std::mutex dart_port_mutex;
std::uintptr_t dart_port_id = 0;
std::map<std::uintptr_t, DartWake> dart_ports;

template <std::size_t Count>
void dart_port_notify(
  void* context, const std::int64_t (&values)[Count]
) noexcept {
  auto wake = std::optional<DartWake>{};
  {
    const auto lock = std::lock_guard{dart_port_mutex};
    const auto found =
      dart_ports.find(reinterpret_cast<std::uintptr_t>(context));
    if (found == dart_ports.end()) return;
    wake = found->second;
  }
  auto messages = std::array<DartIntegerMessage, Count>{};
  auto pointers = std::array<DartIntegerMessage*, Count>{};
  for (auto index = std::size_t{}; index < Count; ++index) {
    messages[index].value = values[index];
    pointers[index] = &messages[index];
  }
  auto message = DartIntegerMessage{};
  message.type = 6;
  message.array = {Count, pointers.data()};
  static_cast<void>(wake->post(wake->port, &message));
}

#include "c_api/callback_port_generated.inc"

struct AdapterArena {
  struct Block {
    Block* previous;
  };
  Block* blocks = nullptr;
  void* cursor = nullptr;
  std::size_t available = 0;
  std::size_t next_capacity = 1024;

  auto allocate(std::size_t size, std::size_t alignment) -> void* {
    size = std::max(size, std::size_t{1});
    if (!cursor || !std::align(alignment, size, cursor, available)) {
      constexpr auto maximum = std::numeric_limits<std::size_t>::max();
      if (
        alignment - 1 > maximum - sizeof(Block) ||
        size > maximum - sizeof(Block) - (alignment - 1)
      )
        throw std::bad_alloc{};
      const auto capacity = std::max(next_capacity, size + alignment - 1);
      auto* block =
        new (::operator new(sizeof(Block) + capacity)) Block{blocks};
      blocks = block;
      cursor = reinterpret_cast<std::byte*>(block) + sizeof(Block);
      available = capacity;
      next_capacity =
        capacity <= (maximum - sizeof(Block)) / 2 ? capacity * 2 : capacity;
      static_cast<void>(std::align(alignment, size, cursor, available));
    }
    auto* result = cursor;
    cursor = static_cast<std::byte*>(cursor) + size;
    available -= size;
    return result;
  }
  std::vector<std::uint64_t> handles;
  std::vector<std::pair<mln_runtime_callback_release, void*>> releases;

  ~AdapterArena() {
    for (const auto& [release, context] : releases) {
      release(context);
    }
    for (const auto handle : handles) {
      const auto* type =
        mln::core::handle_kind_name(mln::core::handle_kind_of(handle));
      static_cast<void>(mln::capture::dispose_owner(type, handle));
    }
    while (blocks) {
      auto* previous = blocks->previous;
      ::operator delete(blocks);
      blocks = previous;
    }
  }
};
struct DartRelease {
  DartWake wake;
  std::uint64_t registration;
  std::unique_ptr<AdapterArena> arena;
};
std::mutex dart_release_mutex;
std::uint64_t dart_release_registration = 0;
std::map<void*, DartRelease> dart_releases;

struct AdapterOwnerToken {
  std::uint64_t handle = 0;
};
using AdapterLogRecordView = mln_adapter_log_record;

std::mutex log_setter_mutex;

struct AdapterCompletionState {
  std::uint32_t copy_kind = MLN_ADAPTER_COMPLETION_COPY_FLAT;
  std::size_t element_size = 0;
  mln_adapter_completion_listener listener = nullptr;
  void* user_data = nullptr;
  std::optional<DartWake> dart_port;
  std::int64_t dart_token = 0;
};

void deliver_completion(
  const AdapterCompletionState& state, AdapterCompletionRecord* record
) noexcept {
  if (!state.dart_port) {
    state.listener(state.user_data, record ? &record->view : nullptr);
    return;
  }
  auto token = DartIntegerMessage{3, {state.dart_token}};
  auto payload = DartIntegerMessage{};
  payload.type = 11;
  payload.native_pointer = {
    reinterpret_cast<std::intptr_t>(record ? &record->view : nullptr), 0,
    [](void*, void* peer) {
      mln_adapter_completion_record_destroy(
        static_cast<mln_adapter_completion_record*>(peer)
      );
    }
  };
  DartIntegerMessage* values[] = {&token, &payload};
  auto message = DartIntegerMessage{};
  message.type = 6;
  message.array = {2, values};
  // The VM owns the payload after serialization, including failed delivery.
  static_cast<void>(state.dart_port->post(state.dart_port->port, &message));
}

// A deferred callback context. Registrations guarantee that release follows
// the final call, so the state stays immutable while calls read it.
struct AdapterDeferredState {
  std::uint32_t callback = 0;
  mln_adapter_deferred_call_listener listener = nullptr;
  void* user_data = nullptr;
  std::optional<DartWake> dart_port;
};

auto create_deferred_state(std::uint32_t callback, void** out_context)
  -> std::unique_ptr<AdapterDeferredState> {
  if (
    mln::capture::deferred_function(callback) == nullptr ||
    out_context == nullptr || *out_context != nullptr
  ) {
    mln::core::set_thread_error("deferred callback arguments are invalid");
    return nullptr;
  }
  auto state = std::make_unique<AdapterDeferredState>();
  state->callback = callback;
  return state;
}

void post_deferred(
  const DartWake& port, std::uint32_t callback,
  mln_adapter_deferred_call_record* record
) noexcept {
  auto kind = DartIntegerMessage{3, {static_cast<std::int64_t>(callback)}};
  auto payload = DartIntegerMessage{};
  payload.type = 11;
  payload.native_pointer = {
    reinterpret_cast<std::intptr_t>(record), 0, [](void*, void* peer) {
      mln_adapter_deferred_call_record_destroy(
        static_cast<mln_adapter_deferred_call_record*>(peer)
      );
    }
  };
  DartIntegerMessage* values[] = {&kind, &payload};
  auto message = DartIntegerMessage{};
  message.type = 6;
  message.array = {2, values};
  // The VM owns the payload after serialization, including failed delivery.
  static_cast<void>(port.post(port.port, &message));
}

auto adapter_completion_callback(
  void* user_data, const mln_completion_result* result
) noexcept -> void {
  const auto* state = static_cast<const AdapterCompletionState*>(user_data);
  if (state == nullptr || result == nullptr || state->listener == nullptr)
    return;
  AdapterCompletionRecord* record = nullptr;
  try {
    record = mln::capture::copy(*result, state->copy_kind, state->element_size);
  } catch (...) {
    mln::capture::discard(state->copy_kind, *result);
    deliver_completion(*state, nullptr);
    return;
  }
  deliver_completion(*state, record);
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

template <class Route>
auto route_matches_url(const Route& route, const mln_resource_request& request)
  -> bool {
  const auto* candidate =
    has_flag(route.flags, MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL)
      ? request.requested_url
      : request.resolved_url;
  return url_matches(route.flags, KnownRouteFlags, route.url, candidate);
}

template <class Route>
auto request_matches_route(
  std::span<const Route> routes, const mln_resource_request& request
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

void destroy_owner_token(void* token) noexcept {
  static_cast<void>(std::unique_ptr<AdapterOwnerToken>{
    static_cast<AdapterOwnerToken*>(token),
  });
}

}  // namespace

auto mln::capture::deliver_deferred(
  void* context, std::uint32_t kind, DeferredRecord* record
) noexcept -> bool {
  const auto* state = static_cast<const AdapterDeferredState*>(context);
  if (state == nullptr || state->callback != kind) return false;
  if (state->dart_port) {
    post_deferred(*state->dart_port, kind, &record->view);
  } else {
    state->listener(state->user_data, &record->view);
  }
  return true;
}

extern "C" MLN_API auto mln_adapter_deferred_callback_create(
  std::uint32_t callback, mln_adapter_deferred_call_listener listener,
  void* listener_user_data, void** out_context
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto state = create_deferred_state(callback, out_context);
    if (state == nullptr || listener == nullptr) {
      mln::core::set_thread_error("deferred callback arguments are invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    state->listener = listener;
    state->user_data = listener_user_data;
    *out_context = state.release();
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_dart_deferred_callback_create(
  std::uint32_t callback, void* post_cobject, std::int64_t port,
  void** out_context
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    auto state = create_deferred_state(callback, out_context);
    if (state == nullptr || post_cobject == nullptr || port == 0) {
      mln::core::set_thread_error("deferred callback arguments are invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    state->dart_port =
      DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port};
    *out_context = state.release();
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_deferred_callback_function(
  std::uint32_t callback
) noexcept -> void* {
  return mln::capture::deferred_function(callback);
}

extern "C" MLN_API void mln_adapter_deferred_callback_release(
  void* context
) noexcept {
  const auto state = std::unique_ptr<AdapterDeferredState>{
    static_cast<AdapterDeferredState*>(context)
  };
  if (state == nullptr) return;
  if (state->dart_port) {
    state->dart_port->notify(0);
  } else {
    state->listener(state->user_data, nullptr);
  }
}

extern "C" MLN_API void mln_adapter_deferred_call_record_adopt(
  mln_adapter_deferred_call_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  static_cast<mln::capture::DeferredRecord*>(record->owner)->claimed = true;
}

extern "C" MLN_API void mln_adapter_deferred_call_record_destroy(
  mln_adapter_deferred_call_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  mln::capture::destroy_deferred(
    static_cast<mln::capture::DeferredRecord*>(record->owner)
  );
}

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
      !mln::capture::valid_kind(copy_kind)
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

extern "C" MLN_API auto mln_adapter_dart_completion_create(
  std::uint32_t copy_kind, std::size_t element_size, void* post_cobject,
  std::int64_t port, std::int64_t token, mln_completion* out_completion
) noexcept -> mln_status {
  if (!post_cobject || !port) return MLN_STATUS_INVALID_ARGUMENT;
  const auto status = mln_adapter_completion_create(
    copy_kind, element_size, [](void*, mln_adapter_completion_record*) {},
    nullptr, out_completion
  );
  if (status != MLN_STATUS_OK) return status;
  auto* state = static_cast<AdapterCompletionState*>(out_completion->user_data);
  state->dart_port =
    DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port};
  state->dart_token = token;
  return MLN_STATUS_OK;
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

extern "C" MLN_API void mln_adapter_completion_record_adopt(
  mln_adapter_completion_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  static_cast<AdapterCompletionRecord*>(record->owner)->claimed = true;
}

extern "C" MLN_API void mln_adapter_completion_record_destroy(
  mln_adapter_completion_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  mln::capture::destroy(static_cast<AdapterCompletionRecord*>(record->owner));
}

extern "C" MLN_API auto mln_adapter_dart_wake_create(
  void* post_cobject, std::int64_t port, mln_wake* out_wake
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (post_cobject == nullptr || port == 0 || out_wake == nullptr) {
      mln::core::set_thread_error(
        "Dart wake requires a posting function, port and output"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto state = std::make_unique<DartWake>(
      DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port}
    );
    *out_wake = {};
    out_wake->size = sizeof(mln_wake);
    out_wake->callback = [](void* context) {
      static_cast<DartWake*>(context)->notify(0);
    };
    out_wake->release_user_data = [](void* context) {
      const auto state =
        std::unique_ptr<DartWake>{static_cast<DartWake*>(context)};
      state->notify(1);
    };
    out_wake->user_data = state.release();
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_dart_port_create(
  void* post_cobject, std::int64_t port
) noexcept -> void* {
  if (!post_cobject || !port) return nullptr;
  try {
    const auto lock = std::lock_guard{dart_port_mutex};
    if (dart_port_id == std::numeric_limits<std::uintptr_t>::max())
      return nullptr;
    const auto id = ++dart_port_id;
    dart_ports.emplace(
      id, DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port}
    );
    return reinterpret_cast<void*>(id);
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API auto mln_adapter_dart_port_function(
  std::uint32_t id
) noexcept -> void* {
  return dart_port_function(id);
}

extern "C" MLN_API void mln_adapter_dart_port_release(void* context) noexcept {
  auto wake = std::optional<DartWake>{};
  {
    const auto lock = std::lock_guard{dart_port_mutex};
    const auto entry =
      dart_ports.find(reinterpret_cast<std::uintptr_t>(context));
    if (entry == dart_ports.end()) return;
    wake = entry->second;
    dart_ports.erase(entry);
  }
  wake->notify(0);
}

extern "C" MLN_API auto mln_adapter_arena_create() noexcept -> void* {
  try {
    return new AdapterArena{};
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API auto mln_adapter_arena_allocate(
  void* arena, std::size_t size, std::size_t alignment
) noexcept -> void* {
  if (!arena || !alignment || (alignment & (alignment - 1))) return nullptr;
  try {
    auto* data = static_cast<AdapterArena*>(arena)->allocate(size, alignment);
    std::memset(data, 0, size);
    return data;
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API void mln_adapter_arena_destroy(void* arena) noexcept {
  delete static_cast<AdapterArena*>(arena);
}

extern "C" MLN_API auto mln_adapter_arena_adopt_handle(
  void* arena, std::uint64_t handle
) noexcept -> mln_status {
  const auto status = mln::c_api::status_boundary([&]() -> mln_status {
    if (!arena) return MLN_STATUS_INVALID_ARGUMENT;
    static_cast<AdapterArena*>(arena)->handles.push_back(handle);
    return MLN_STATUS_OK;
  });
  if (status != MLN_STATUS_OK) {
    const auto* type =
      mln::core::handle_kind_name(mln::core::handle_kind_of(handle));
    static_cast<void>(mln::capture::dispose_owner(type, handle));
  }
  return status;
}

extern "C" MLN_API auto mln_adapter_arena_adopt_release(
  void* arena, mln_runtime_callback_release release, void* context
) noexcept -> mln_status {
  if (release == nullptr) return MLN_STATUS_INVALID_ARGUMENT;
  const auto status = mln::c_api::status_boundary([&]() -> mln_status {
    if (!arena) return MLN_STATUS_INVALID_ARGUMENT;
    static_cast<AdapterArena*>(arena)->releases.emplace_back(release, context);
    return MLN_STATUS_OK;
  });
  if (status != MLN_STATUS_OK) release(context);
  return status;
}

extern "C" MLN_API auto mln_adapter_dart_release_register(
  void* post_cobject, std::int64_t port, void* context, void* arena,
  std::uint64_t* out_registration
) noexcept -> mln_status {
  auto owned_arena =
    std::unique_ptr<AdapterArena>{static_cast<AdapterArena*>(arena)};
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (out_registration) *out_registration = 0;
    if (
      post_cobject == nullptr || port == 0 || context == nullptr ||
      !out_registration
    ) {
      mln::core::set_thread_error(
        "Dart release requires a posting function, port, context and output"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto lock = std::lock_guard{dart_release_mutex};
    if (dart_release_registration == std::numeric_limits<std::int64_t>::max()) {
      mln::core::set_thread_error(
        "Dart release registration identifiers exhausted"
      );
      return MLN_STATUS_NATIVE_ERROR;
    }
    const auto id = ++dart_release_registration;
    const auto [entry, inserted] = dart_releases.try_emplace(context);
    if (!inserted) {
      mln::core::set_thread_error("Dart release context is already registered");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    entry->second = DartRelease{
      DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port}, id,
      std::move(owned_arena)
    };
    *out_registration = id;
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API void mln_adapter_dart_release(void* context) noexcept {
  auto notification = std::optional<DartRelease>{};
  {
    const auto lock = std::lock_guard{dart_release_mutex};
    const auto entry = dart_releases.find(context);
    if (entry == dart_releases.end()) return;
    notification = std::move(entry->second);
    dart_releases.erase(entry);
  }
  notification->wake.notify(
    static_cast<std::int64_t>(notification->registration)
  );
}

extern "C" MLN_API auto mln_adapter_owner_token_create(
  std::uint64_t handle
) noexcept -> void* {
  try {
    auto token = std::make_unique<AdapterOwnerToken>();
    token->handle = handle;
    return token.release();
  } catch (...) {
    const auto* type =
      mln::core::handle_kind_name(mln::core::handle_kind_of(handle));
    static_cast<void>(mln::capture::dispose_owner(type, handle));
    return nullptr;
  }
}

extern "C" MLN_API void mln_adapter_owner_token_destroy(void* token) noexcept {
  destroy_owner_token(token);
}

extern "C" MLN_API void mln_adapter_owner_finalize(void* token) noexcept {
  auto* owner = static_cast<AdapterOwnerToken*>(token);
  if (owner != nullptr) {
    const auto* type =
      mln::core::handle_kind_name(mln::core::handle_kind_of(owner->handle));
    const auto status = mln::capture::dispose_owner(type, owner->handle);
    if (status != MLN_STATUS_OK) {
      static_cast<void>(std::fprintf(
        stderr,
        "maplibre_native_ffi: owner finalization failed for handle %llu "
        "(status %d)\n",
        static_cast<unsigned long long>(owner->handle), static_cast<int>(status)
      ));
    }
  }
  destroy_owner_token(token);
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

extern "C" MLN_API auto mln_adapter_routed_resource_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) noexcept -> std::uint32_t {
  // Each route decides which URL it compares, so route matching handles an
  // absent URL.
  if (user_data == nullptr || request == nullptr || handle == MLN_HANDLE_NULL) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  const auto& provider =
    *static_cast<const mln_adapter_routed_resource_provider*>(user_data);
  if (
    provider.callback == nullptr ||
    (provider.routes == nullptr && provider.route_count != 0) ||
    !request_matches_route(
      std::span{provider.routes, provider.route_count}, *request
    )
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  return provider.callback(provider.user_data, request, handle);
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
