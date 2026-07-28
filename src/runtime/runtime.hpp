#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <mbgl/storage/resource_options.hpp>
#include <mbgl/util/run_loop.hpp>

#include "maplibre_native_c.h"

namespace mbgl {
class DatabaseFileSource;
}  // namespace mbgl

namespace mln::core {

struct ResourceProvider {
  mln_resource_provider_callback callback = nullptr;
  void* user_data = nullptr;
};

// Holds the resource transform registration in its own reference-counted
// object so a file-source lookup can keep the registration alive on its own.
//
// A lookup copies the runtime's handle to this state while it holds the
// process-global runtime registry lock, which proves the runtime is live, and
// takes `mutex` after releasing that lock. The copy is what keeps the state
// readable, so the registry lock covers a non-blocking pointer copy instead of
// a lock acquisition that a pending writer can delay.
//
// `callback` and `user_data` live here rather than on the runtime, so a lease
// holder reads and invokes the registration without touching the runtime.
// Runtime teardown clears both under the exclusive lock: a callback already
// inside the shared lock keeps teardown waiting until it returns, and a lease
// that takes the shared lock later observes an empty registration.
struct ResourceTransformState {
  std::shared_mutex mutex;
  mln_resource_transform_callback callback = nullptr;
  void* user_data = nullptr;
};

// Holds the resource provider registration in its own reference-counted object,
// mirroring `ResourceTransformState`.
//
// A file-source lookup copies the runtime's handle to this state under the
// process-global registry lock, which proves the runtime is live, and takes
// `mutex` only after releasing it. Acquiring the shared lock while still
// holding the registry lock would queue behind a pending writer and stall every
// unrelated runtime for the duration of the in-flight callback, because
// `validate_runtime()` and every other file-source lookup need that same
// process-global mutex.
struct ResourceProviderState {
  std::shared_mutex mutex;
  bool registered = false;
  ResourceProvider provider;
};

// Borrows the resource provider registered on a runtime for the duration of one
// provider callback. The lease holds a shared lock on the state's mutex, so
// `set_resource_provider()`, `clear_resource_provider()`, and
// `destroy_runtime()` wait for every live lease before they retire a callback
// and its `user_data`. The lease also retains the state itself, so it stays
// readable even after the runtime it came from is gone.
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
  mln_runtime* runtime = nullptr;
  bool alive = false;
};

// Holds the latch a parked owner thread waits on, in its own reference-counted
// object so a wake source keeps it readable after the runtime is destroyed.
//
// `mutex` is a leaf lock. Signalling takes it while MapLibre holds the
// `RunLoop` mutex or the runtime holds `event_mutex`, so those two order ahead
// of it everywhere.
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
  void* source;
  mln_map* map;
  int32_t code;
  uint32_t payload_type;
  std::vector<std::byte> payload;
  std::string message;
  bool has_offline_region = false;
  mln_offline_region_id offline_region_id = 0;
  bool has_offline_operation = false;
  mln_offline_operation_id offline_operation_id = 0;
};

}  // namespace mln::core

struct mln_runtime {
  std::thread::id owner_thread;
  std::unique_ptr<mbgl::util::RunLoop> run_loop;
  std::string asset_path;
  std::string cache_path;
  std::shared_ptr<mbgl::DatabaseFileSource> database_source;
  bool has_maximum_cache_size = false;
  std::uint64_t maximum_cache_size = 0;
  std::shared_ptr<mln::core::ResourceProviderState> resource_provider_state;
  std::shared_ptr<mln::core::OfflineRegionEventState> offline_event_state;
  std::shared_ptr<mln::core::OfflineOperationEventState>
    offline_operation_state;
  std::shared_ptr<mln::core::ResourceTransformState> resource_transform_state;
  std::shared_ptr<mln::core::WakeState> wake_state;
  std::size_t live_maps = 0;
  mutable std::mutex event_mutex;
  std::unordered_set<const mln_map*> event_maps;
  std::deque<mln::core::QueuedRuntimeEvent> events;
  std::unordered_set<mln_offline_region_id> observed_offline_regions;
  std::vector<std::byte> last_polled_event_payload;
  std::string last_polled_event_message;
  std::unordered_map<const mln_map*, std::string> map_loading_failures;
};

namespace mln::core {

auto create_runtime(
  const mln_runtime_options* options, mln_runtime** out_runtime
) -> mln_status;
auto destroy_runtime(mln_runtime* runtime) -> mln_status;
auto pump_runtime(mln_runtime* runtime, int64_t timeout_ms) -> mln_status;
auto acquire_wake_source(mln_runtime* runtime, mln_wake_source** out_source)
  -> mln_status;
auto signal_wake_source(mln_wake_source* source) -> mln_status;
auto destroy_wake_source(mln_wake_source* source) noexcept -> void;
// Latches a wake for the runtime owning `state` and releases any parked owner
// thread. Callers hold the `RunLoop` mutex or `event_mutex`; see `WakeState`.
auto signal_wake(const std::shared_ptr<WakeState>& state) noexcept -> void;
auto poll_runtime_event(
  mln_runtime* runtime, mln_runtime_event* out_event, bool* out_has_event
) -> mln_status;
auto set_resource_provider(
  mln_runtime* runtime, const mln_resource_provider* provider
) -> mln_status;
auto clear_resource_provider(mln_runtime* runtime) -> mln_status;
auto set_resource_transform(
  mln_runtime* runtime, const mln_resource_transform* transform
) -> mln_status;
auto resource_transform_response_set_url(
  mln_resource_transform_response* response, const char* url, size_t url_size
) -> mln_status;
auto clear_resource_transform(mln_runtime* runtime) -> mln_status;
auto invoke_resource_transform(
  void* platform_context, uint32_t kind, const char* url,
  std::string& out_replacement_url
) noexcept -> mln_status;
auto run_ambient_cache_operation_start(
  mln_runtime* runtime, uint32_t operation,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_operation_discard(
  mln_runtime* runtime, mln_offline_operation_id operation_id
) -> mln_status;
auto offline_region_create_start(
  mln_runtime* runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_get_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_regions_list_start(
  mln_runtime* runtime, mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_regions_merge_database_start(
  mln_runtime* runtime, const char* side_database_path,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_update_metadata_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  const uint8_t* metadata, size_t metadata_size,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_get_status_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_set_observed_start(
  mln_runtime* runtime, mln_offline_region_id region_id, bool observed,
  mln_offline_operation_id* out_operation_id
) -> mln_status;

struct OfflineRegionDownloadStateRequest {
  mln_offline_region_id region_id;
  uint32_t state;
};

auto offline_region_set_download_state_start(
  mln_runtime* runtime, OfflineRegionDownloadStateRequest request,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_invalidate_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_delete_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status;
auto offline_region_create_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot** out_region
) -> mln_status;
auto offline_region_get_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot** out_region, bool* out_found
) -> mln_status;
auto offline_regions_list_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list** out_regions
) -> mln_status;
auto offline_regions_merge_database_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list** out_regions
) -> mln_status;
auto offline_region_update_metadata_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot** out_region
) -> mln_status;
auto offline_region_get_status_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_status* out_status
) -> mln_status;
auto offline_region_snapshot_get(
  const mln_offline_region_snapshot* snapshot, mln_offline_region_info* out_info
) -> mln_status;
auto offline_region_snapshot_destroy(
  mln_offline_region_snapshot* snapshot
) noexcept -> void;
auto offline_region_list_count(
  const mln_offline_region_list* list, size_t* out_count
) -> mln_status;
auto offline_region_list_get(
  const mln_offline_region_list* list, size_t index,
  mln_offline_region_info* out_info
) -> mln_status;
auto offline_region_list_destroy(mln_offline_region_list* list) noexcept
  -> void;
auto retain_runtime_map(mln_runtime* runtime) -> mln_status;
auto release_runtime_map(mln_runtime* runtime) noexcept -> void;
auto validate_runtime(mln_runtime* runtime) -> mln_status;
auto resource_options_for_runtime(mln_runtime* runtime)
  -> mbgl::ResourceOptions;
// Leases the resource provider registered on the runtime named by a MapLibre
// platform context. Acquiring the shared provider lock while the registry lock
// is held hands runtime lifetime safely to the caller. Hold the returned lease
// across the provider callback, so replacement and teardown cannot retire its
// callback or `user_data`. Returns nullopt when the platform context names no
// live runtime or the runtime carries no provider.
auto acquire_resource_provider_for_platform_context(
  void* platform_context
) noexcept -> std::optional<ResourceProviderLease>;

// Copies the maximum ambient cache size configured on the runtime named by a
// MapLibre platform context, under the same registry lock. Returns nullopt when
// the platform context names no live runtime or the runtime carries no maximum.
auto find_maximum_cache_size_for_platform_context(
  void* platform_context
) noexcept -> std::optional<std::uint64_t>;

// Reports whether a resource transform is registered. MapLibre-owned threads
// observe a value instead of a runtime pointer teardown may retire, and the
// process-global registry lock is released before the per-runtime transform
// lock is taken. See `ResourceTransformState`.
auto has_resource_transform_for_platform_context(
  void* platform_context
) noexcept -> bool;
auto push_runtime_map_event(
  mln_runtime* runtime, mln_map* map, uint32_t type, int32_t code = 0,
  const char* message = nullptr
) -> void;
auto push_runtime_map_event_payload(
  mln_runtime* runtime, mln_map* map, uint32_t type, uint32_t payload_type,
  std::vector<std::byte> payload, int32_t code = 0, std::string message = {}
) -> void;
auto register_runtime_map_events(mln_runtime* runtime, const mln_map* map)
  -> void;
auto clear_runtime_map_loading_failure(mln_runtime* runtime, const mln_map* map)
  -> void;
auto runtime_map_loading_failed(mln_runtime* runtime, const mln_map* map)
  -> bool;
auto runtime_map_loading_failure_message(
  mln_runtime* runtime, const mln_map* map
) -> std::string;
auto discard_runtime_map_events(mln_runtime* runtime, const mln_map* map)
  -> void;

}  // namespace mln::core
