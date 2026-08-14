#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <mbgl/storage/resource_options.hpp>
#include <mbgl/util/run_loop.hpp>

#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mbgl {
class DatabaseFileSource;
}  // namespace mbgl

namespace mln::core {

struct RuntimeObject;

// Read on the map owner thread by every map event producer, and on the mbgl
// DatabaseFileSourceThread by every offline producer. Held by shared_ptr so a
// camera transition that outlives its map still reads a live cell.
struct MapEventState {
  std::atomic<uint64_t> mask;
  // Owner-thread only. The style setters clear this before a load and read it
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

struct ResourceProvider {
  mln_resource_provider_callback callback = nullptr;
  void* user_data = nullptr;
};

// Holds the resource transform registration in a reference-counted object that
// outlives the runtime. A file-source lookup copies the runtime's pointer to
// this state under the process-global runtime registry lock and takes `mutex`
// only after releasing that lock; taking `mutex` under the registry lock would
// stall every unrelated runtime for the duration of an in-flight callback.
// Runtime teardown clears the registration under the exclusive lock.
struct ResourceTransformState {
  std::shared_mutex mutex;
  mln_resource_transform_callback callback = nullptr;
  void* user_data = nullptr;
};

struct HttpHeaderTransformState {
  std::shared_mutex mutex;
  mln_http_header_transform_callback callback = nullptr;
  void* user_data = nullptr;
};

using HttpHeader = std::pair<std::string, std::string>;
using HttpHeaders = std::vector<HttpHeader>;

// Holds the resource provider registration under the same locking rule as
// `ResourceTransformState`.
struct ResourceProviderState {
  std::shared_mutex mutex;
  bool registered = false;
  ResourceProvider provider;
};

// Borrows the resource provider registered on a runtime for the duration of one
// provider callback. `set_resource_provider()`, `clear_resource_provider()`,
// and `destroy_runtime()` wait for every live lease before retiring a callback
// and its `user_data`. The lease retains the state, so it stays readable after
// the runtime is gone.
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
  [[nodiscard]] auto user_data() const -> void* { return provider_.user_data; }

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

// Holds the wake flag and the condition variable a parked owner thread blocks
// on. Reference-counted so a wake source keeps it readable after the runtime is
// destroyed.
//
// `mutex` is a leaf lock: the `RunLoop` mutex and `event_mutex` both order
// ahead of it.
struct WakeState {
  std::mutex mutex;
  std::condition_variable condition;
  bool signaled = false;
  bool alive = true;
};

struct OfflineOperationEventState;

struct QueuedRuntimeEvent {
  uint32_t type;
  uint32_t source_type;
  // The mln_map for map-originated events, the mln_runtime otherwise, as
  // selected by source_type.
  uint64_t source;
  int32_t code;
  uint32_t payload_type;
  mln_runtime_event_payload payload = zeroed_event_payload();
  std::string message;
  bool has_offline_region = false;
  mln_offline_region_id offline_region_id = 0;
  bool has_offline_operation = false;
  mln_offline_operation_id offline_operation_id = 0;
};

struct RuntimeObject {
  mln_runtime self = MLN_HANDLE_NULL;
  // The token this runtime hands to mbgl as its opaque platform context.
  void* platform_context = nullptr;
  std::thread::id owner_thread;
  std::unique_ptr<mbgl::util::RunLoop> run_loop;
  std::string asset_path;
  std::string cache_path;
  std::shared_ptr<mbgl::DatabaseFileSource> database_source;
  std::shared_ptr<mln::core::ResourceProviderState> resource_provider_state;
  std::shared_ptr<mln::core::OfflineRegionEventState> offline_event_state;
  std::shared_ptr<mln::core::OfflineOperationEventState>
    offline_operation_state;
  std::shared_ptr<mln::core::ResourceTransformState> resource_transform_state;
  std::shared_ptr<mln::core::HttpHeaderTransformState>
    http_header_transform_state;
  std::shared_ptr<mln::core::WakeState> wake_state;
  std::shared_ptr<mln::core::RuntimeEventState> event_state;
  std::size_t live_maps = 0;
  mutable std::mutex event_mutex;
  std::unordered_set<mln_map> event_maps;
  std::deque<mln::core::QueuedRuntimeEvent> events;
  std::unordered_set<mln_offline_region_id> observed_offline_regions;
  // Owner-thread only. A drain is the only reader and writer, and every drain
  // is owner-thread affine, so event_mutex does not guard these three. Each
  // keeps its capacity, so a steady-state drain allocates nothing.
  std::vector<mln::core::QueuedRuntimeEvent> event_drain_staging;
  std::vector<mln_runtime_event> event_batch_events;
  std::string event_batch_messages;
  // Owner-thread only: the live pump's drain budget. The run loop's process
  // gate reads these between tasks, and the pump writes them around each
  // drain, all on the owner thread. Unset outside a bounded pump.
  std::optional<std::chrono::steady_clock::time_point> pump_deadline;
  bool pump_ran_task = false;
  bool pump_budget_exhausted = false;
};

template <>
struct HandleTraits<RuntimeObject> {
  static constexpr auto kind = HandleKind::Runtime;
  // Run-loop join and file-source teardown are owner-thread work, so a lease
  // must not let a foreign thread outlive destroy_runtime().
  static constexpr auto leasable = false;
};

auto create_runtime(
  const mln_runtime_options* options, mln_runtime* out_runtime
) -> mln_status;
auto destroy_runtime(mln_runtime runtime) -> mln_status;
auto pump_runtime(mln_runtime runtime, int64_t timeout_ms, int64_t budget_ms)
  -> mln_status;
auto acquire_wake_source(mln_runtime runtime, mln_wake_source* out_source)
  -> mln_status;
auto signal_wake_source(mln_wake_source source) -> mln_status;
auto destroy_wake_source(mln_wake_source source) noexcept -> void;
// Sets the wake flag for the runtime owning `state` and releases any parked
// owner thread. Callers hold the `RunLoop` mutex or `event_mutex`; see
// `WakeState`.
auto signal_wake(const std::shared_ptr<WakeState>& state) noexcept -> void;
auto drain_runtime_events(
  mln_runtime runtime, size_t max_events, mln_runtime_event_batch* out_batch
) -> mln_status;
auto set_runtime_event_mask(mln_runtime runtime, uint64_t mask) -> mln_status;
auto get_runtime_event_mask(mln_runtime runtime, uint64_t* out_mask)
  -> mln_status;
auto set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider
) -> mln_status;
auto clear_resource_provider(mln_runtime runtime) -> mln_status;
auto set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform
) -> mln_status;
auto resource_transform_response_set_url(
  mln_resource_transform_response* response, const char* url, size_t url_size
) -> mln_status;
auto clear_resource_transform(mln_runtime runtime) -> mln_status;
auto set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform
) -> mln_status;
auto validate_http_header(
  const char* name, size_t name_size, const char* value, size_t value_size
) -> mln_status;
auto http_header_transform_response_set(
  mln_http_header_transform_response* response, const char* name,
  size_t name_size, const char* value, size_t value_size
) -> mln_status;
auto clear_http_header_transform(mln_runtime runtime) -> mln_status;
auto invoke_http_header_transform(
  void* platform_context, uint32_t kind, const char* url
) noexcept -> HttpHeaders;
auto invoke_resource_transform(
  void* platform_context, uint32_t kind, const char* url,
  std::string& out_replacement_url
) noexcept -> mln_status;
auto run_ambient_cache_operation_start(
  mln_runtime runtime, uint32_t operation,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto set_maximum_ambient_cache_size_start(
  mln_runtime runtime, std::uint64_t size,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_operation_discard(
  mln_runtime runtime, mln_offline_operation_id operation_id
) -> mln_status;
auto offline_region_create_start(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_get_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_regions_list_start(
  mln_runtime runtime, mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_regions_merge_database_start(
  mln_runtime runtime, const char* side_database_path,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_update_metadata_start(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_get_status_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_set_observed_start(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  mln_offline_operation_id* out_operation_id
) -> mln_status;

struct OfflineRegionDownloadStateRequest {
  mln_offline_region_id region_id;
  uint32_t state;
};

auto offline_region_set_download_state_start(
  mln_runtime runtime, OfflineRegionDownloadStateRequest request,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_invalidate_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_delete_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_create_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region
) -> mln_status;
auto offline_region_get_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region, bool* out_found
) -> mln_status;
auto offline_regions_list_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list* out_regions
) -> mln_status;
auto offline_regions_merge_database_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list* out_regions
) -> mln_status;
auto offline_region_update_metadata_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region
) -> mln_status;
auto offline_region_get_status_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_status* out_status
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

// The run loop this runtime pumps from mln_runtime_pump(). Use it as the
// mbgl::Scheduler backing a Mailbox, so work posted from a foreign thread is
// delivered on the runtime owner thread. Prefer this over
// mbgl::util::RunLoop::Get(), which reads the calling thread's ambient
// scheduler.
auto runtime_run_loop(RuntimeObject* runtime) -> mbgl::util::RunLoop&;

auto resource_options_for_runtime(mln_runtime runtime) -> mbgl::ResourceOptions;
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
auto register_runtime_map_events(mln_runtime runtime, mln_map map) -> void;
auto discard_runtime_map_events(mln_runtime runtime, mln_map map) -> void;

}  // namespace mln::core
