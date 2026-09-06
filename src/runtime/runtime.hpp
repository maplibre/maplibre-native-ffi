#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "execution/control_state.hpp"
#include "execution/runtime_executor.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mln {
class DatabaseFileSource;
class ResourceOptions;
namespace util {
class RunLoop;
}  // namespace util
}  // namespace mln

namespace mln::core {

struct RuntimeObject;
class Wake;
class OperationObject;
class Completion;

// Read on the runtime worker by map event producers, and on the mbgl
// DatabaseFileSourceThread by offline producers. Held by shared_ptr so a camera
// transition that outlives its map still reads a live cell.
struct MapEventState {
  std::atomic<uint64_t> mask;
  // Runtime-worker only. The style setters clear this before a load and read it
  // after, so a subscription mask can never change their return status.
  std::string style_load_failure;
  bool style_load_failed = false;
};

struct RuntimeEventState {
  std::atomic<uint64_t> mask;
};

static_assert(
  std::atomic<uint64_t>::is_always_lock_free,
  "a lock-based 64-bit atomic would put a lock on the event producer hot path"
);
static_assert(
  MLN_RUNTIME_EVENT_MASK_ALL == (MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS |
                                 MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS),
  "every event type needs a mask bit in one of the two groups"
);
static_assert(
  MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED < 64,
  "an event type value past 63 needs a wider subscription mask"
);

// One relaxed load, one shift, one test. No handle-table lock, no event_mutex.
// Relaxed ordering is correct because the mask is policy rather than a
// synchronisation edge: the documented semantics gate events queued after a
// setter returns and say nothing about events already in flight.
[[nodiscard]] inline auto event_selected(
  const std::atomic<uint64_t>& mask, uint32_t type
) noexcept -> bool {
  return ((mask.load(std::memory_order_relaxed) >> type) & 1U) != 0U;
}

// A payload whose bytes are all zero. Producers write one member onto it, so an
// event carries no indeterminate bytes past the member its payload type names,
// and an event without a payload carries zeros.
[[nodiscard]] inline auto zeroed_event_payload() noexcept
  -> mln_runtime_event_payload {
  auto payload = mln_runtime_event_payload{};
  std::memset(&payload, 0, sizeof(payload));
  return payload;
}

class RuntimeCallbackContext {
 public:
  RuntimeCallbackContext(
    void* user_data, mln_runtime_callback_release release
  ) noexcept
      : user_data_(user_data), release_(release) {}

  RuntimeCallbackContext(const RuntimeCallbackContext&) = delete;
  auto operator=(const RuntimeCallbackContext&)
    -> RuntimeCallbackContext& = delete;

  ~RuntimeCallbackContext() noexcept {
    if (!owned_.load(std::memory_order_acquire) || release_ == nullptr) return;
    try {
      release_(user_data_);
    } catch (...) {
    }
  }

  void transfer_to_runtime() noexcept {
    owned_.store(true, std::memory_order_release);
  }
  void return_to_caller() noexcept {
    owned_.store(false, std::memory_order_release);
  }

  [[nodiscard]] auto user_data() const noexcept -> void* { return user_data_; }

 private:
  void* user_data_ = nullptr;
  mln_runtime_callback_release release_ = nullptr;
  std::atomic_bool owned_{false};
};

struct ResourceProvider {
  mln_resource_provider_callback callback = nullptr;
  std::shared_ptr<RuntimeCallbackContext> context;
};

struct ResourceTransformRegistration {
  mln_resource_transform_callback callback = nullptr;
  std::shared_ptr<RuntimeCallbackContext> context;
};

struct HttpHeaderTransformRegistration {
  mln_http_header_transform_callback callback = nullptr;
  std::shared_ptr<RuntimeCallbackContext> context;
};

// Holds the resource transform registration in a reference-counted object that
// outlives the runtime. A file-source lookup copies the runtime's pointer to
// this state under the process-global runtime registry lock and takes `mutex`
// only after releasing that lock; taking `mutex` under the registry lock would
// stall every unrelated runtime for the duration of an in-flight callback.
// Runtime teardown clears the registration under the exclusive lock.
struct ResourceTransformState {
  std::shared_mutex mutex;
  std::shared_ptr<ResourceTransformRegistration> registration;
};

struct HttpHeaderTransformState {
  std::shared_mutex mutex;
  std::shared_ptr<HttpHeaderTransformRegistration> registration;
};

using HttpHeader = std::pair<std::string, std::string>;
using HttpHeaders = std::vector<HttpHeader>;

// Holds the resource provider registration under the same locking rule as
// `ResourceTransformState`.
struct ResourceProviderState {
  std::shared_mutex mutex;
  ResourceProvider provider;
};

// Borrows the resource provider registered on a runtime for one callback. A
// replacement or clear command waits for every live lease on the runtime
// executor before it emits its terminal event. The lease retains the state, so
// it stays readable after the runtime handle is retired.
class ResourceProviderLease {
 public:
  ResourceProviderLease(
    std::shared_ptr<ResourceProviderState> state,
    std::shared_lock<std::shared_mutex> lock, ResourceProvider provider
  )
      : state_(std::move(state)), lock_(std::move(lock)), provider_(provider) {}

  [[nodiscard]] auto callback() const -> mln_resource_provider_callback {
    return provider_.callback;
  }
  [[nodiscard]] auto user_data() const -> void* {
    return provider_.context->user_data();
  }

 private:
  std::shared_ptr<ResourceProviderState> state_;
  std::shared_lock<std::shared_mutex> lock_;
  ResourceProvider provider_;
};

struct OfflineRegionEventState {
  std::mutex mutex;
  RuntimeObject* runtime = nullptr;
  bool alive = false;
};

struct RuntimeEventStorage {
  std::vector<mln_runtime_event> events;
  std::string messages;
};

struct RuntimeEventQueueState {
  std::mutex mutex;
  std::unordered_map<mln_map, std::shared_ptr<MapEventState>> event_maps;
  RuntimeEventStorage pending;
  std::unordered_set<mln_offline_region_id> observed_offline_regions;
  std::shared_ptr<mln::core::Wake> wake;
};

struct EventBatchObject {
  RuntimeEventStorage storage;
};

struct RuntimeObject {
  mln_runtime self = MLN_HANDLE_NULL;
  // The token this runtime hands to mbgl as its opaque platform context.
  void* platform_context = nullptr;
  ControlState control;
  RuntimeExecutor executor;
  // The single commit point for commands, operations, barriers, and close.
  std::mutex submission_mutex;
  std::mutex terminal_mutex;
  std::condition_variable terminal_condition;
  uint64_t next_submission_sequence = 1;
  std::set<uint64_t> pending_submissions;
  std::map<uint64_t, std::shared_ptr<mln::core::OperationObject>>
    pending_barriers;
  std::string asset_path;
  std::string cache_path;
  std::shared_ptr<mln::DatabaseFileSource> database_source;
  std::shared_ptr<mln::core::ResourceProviderState> resource_provider_state;
  std::shared_ptr<mln::core::OfflineRegionEventState> offline_event_state;
  std::shared_ptr<mln::core::ResourceTransformState> resource_transform_state;
  std::shared_ptr<mln::core::HttpHeaderTransformState>
    http_header_transform_state;
  std::shared_ptr<mln::core::RuntimeEventState> event_state;
  std::shared_ptr<mln::core::RuntimeEventQueueState> event_queue;
};

template <>
struct HandleTraits<RuntimeObject> {
  static constexpr auto kind = HandleKind::Runtime;
  static constexpr auto leasable = true;
};

auto create_runtime(
  const mln_runtime_options* options, mln_runtime* out_runtime
) -> mln_status;
auto runtime_barrier_start(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status;
auto release_runtime(mln_runtime runtime, const mln_completion* completion)
  -> mln_status;
auto drain_runtime_events(mln_runtime runtime, mln_event_batch* out_batch)
  -> mln_status;
auto get_event_batch(
  mln_event_batch batch, mln_runtime_event_batch_view* out_view
) -> mln_status;
auto release_event_batch(mln_event_batch batch) noexcept -> void;
auto set_runtime_event_mask(mln_runtime runtime, uint64_t mask) -> mln_status;
auto get_runtime_event_mask(mln_runtime runtime, uint64_t* out_mask)
  -> mln_status;
auto set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider,
  const mln_completion* completion
) -> mln_status;
auto clear_resource_provider(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status;
auto set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform,
  const mln_completion* completion
) -> mln_status;
auto resource_transform_response_set_url(
  mln_resource_transform_response* response, const char* url, size_t url_size
) -> mln_status;
auto clear_resource_transform(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status;
auto set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  const mln_completion* completion
) -> mln_status;
auto validate_http_header(
  const char* name, size_t name_size, const char* value, size_t value_size
) -> mln_status;
auto http_header_transform_response_set(
  mln_http_header_transform_response* response, const char* name,
  size_t name_size, const char* value, size_t value_size
) -> mln_status;
auto clear_http_header_transform(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status;
auto invoke_http_header_transform(
  void* platform_context, uint32_t kind, const char* url
) noexcept -> HttpHeaders;
auto invoke_resource_transform(
  void* platform_context, uint32_t kind, const char* url,
  std::string& out_replacement_url
) noexcept -> mln_status;
auto run_ambient_cache_operation_start(
  mln_runtime runtime, uint32_t operation, const mln_completion* completion
) -> mln_status;
auto set_maximum_ambient_cache_size_start(
  mln_runtime runtime, std::uint64_t size, const mln_completion* completion
) -> mln_status;
auto offline_region_create_start(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  const mln_completion* completion
) -> mln_status;
auto offline_region_get_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) -> mln_status;
auto offline_regions_list_start(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status;
auto offline_regions_merge_database_start(
  mln_runtime runtime, const char* side_database_path,
  const mln_completion* completion
) -> mln_status;
auto offline_region_update_metadata_start(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, const mln_completion* completion
) -> mln_status;
auto offline_region_get_status_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) -> mln_status;
auto offline_region_set_observed_start(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  const mln_completion* completion
) -> mln_status;

struct OfflineRegionDownloadStateRequest {
  mln_offline_region_id region_id;
  uint32_t state;
};

auto offline_region_set_download_state_start(
  mln_runtime runtime, OfflineRegionDownloadStateRequest request,
  const mln_completion* completion
) -> mln_status;
auto offline_region_invalidate_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) -> mln_status;
auto offline_region_delete_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) -> mln_status;
auto offline_region_snapshot_get(
  mln_offline_region_snapshot snapshot, mln_offline_region_info* out_info
) -> mln_status;
auto offline_region_snapshot_destroy(
  mln_offline_region_snapshot snapshot
) noexcept -> void;
auto offline_region_list_count(mln_offline_region_list list, size_t* out_count)
  -> mln_status;
auto offline_region_list_get(
  mln_offline_region_list list, size_t index, mln_offline_region_info* out_info
) -> mln_status;
auto offline_region_list_destroy(mln_offline_region_list list) noexcept -> void;
auto retain_runtime_map(mln_runtime runtime) -> mln_status;
auto release_runtime_map(mln_runtime runtime) noexcept -> void;
auto validate_runtime(mln_runtime runtime, RuntimeObject*& out_runtime)
  -> mln_status;
[[nodiscard]] auto lease_runtime(mln_runtime runtime)
  -> std::shared_ptr<RuntimeObject>;
auto validate_offline_side_database_path(const char* side_database_path)
  -> mln_status;
auto dispatch_runtime_sync(
  mln_runtime runtime, std::function<mln_status()> function
) -> mln_status;
auto submit_runtime_command(
  const std::shared_ptr<RuntimeObject>& runtime,
  std::function<void(uint64_t)> function,
  const std::shared_ptr<Completion>& completion,
  std::atomic<uint64_t>* latest_submission = nullptr
) -> mln_status;
auto submit_runtime_operation(
  const std::shared_ptr<RuntimeObject>& runtime,
  const std::shared_ptr<OperationObject>& operation,
  std::function<void()> function
) -> mln_status;
auto associate_runtime_operation_with_current_submission(
  RuntimeObject* runtime, const std::shared_ptr<OperationObject>& operation
) noexcept -> bool;

// The continuously running run loop owned by this runtime's executor.
auto runtime_run_loop(RuntimeObject* runtime) -> mln::util::RunLoop&;

auto resource_options_for_runtime(mln_runtime runtime) -> mln::ResourceOptions;
// Leases the resource provider registered on the runtime named by a MapLibre
// platform context. Hold the returned lease across the provider callback, so
// replacement and teardown cannot retire its callback or `user_data`. Returns
// nullopt when the platform context names no live runtime or the runtime
// carries no provider.
auto acquire_resource_provider_for_platform_context(
  void* platform_context
) noexcept -> std::optional<ResourceProviderLease>;

// Reports whether a resource transform is registered. Returns a value rather
// than a runtime pointer teardown may retire, so a MapLibre-owned thread can
// call it safely.
auto has_resource_transform_for_platform_context(
  void* platform_context
) noexcept -> bool;
auto push_runtime_map_event(
  mln_runtime runtime, mln_map map, uint32_t type, int32_t code = 0,
  const char* message = nullptr
) -> void;
auto push_runtime_map_event_payload(
  mln_runtime runtime, mln_map map, uint32_t type, uint32_t payload_type,
  const mln_runtime_event_payload& payload, int32_t code = 0,
  std::string message = {}
) -> void;
auto register_runtime_map_events(
  mln_runtime runtime, mln_map map, std::shared_ptr<MapEventState> event_state
) -> void;
auto unregister_runtime_map_events(mln_runtime runtime, mln_map map) -> void;

}  // namespace mln::core
