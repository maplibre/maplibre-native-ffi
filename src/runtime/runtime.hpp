#pragma once

#include <any>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <mln/storage/resource_transform.hpp>

#include "completion/completion.hpp"
#include "execution/control_state.hpp"
#include "execution/process_exit.hpp"
#include "execution/runtime_executor.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "resources/url_rules.hpp"

namespace mln {
class DatabaseFileSource;
class ResourceOptions;
namespace delivery {
class QueuedRegistration;
}  // namespace delivery
namespace util {
class RunLoop;
}  // namespace util
}  // namespace mln

namespace mln::core {

class CameraTransitionEnd;
struct MapObject;
struct RuntimeObject;
struct RuntimeEventState;
class OperationObject;
class Completion;

// A map event raised inside a map transaction. It waits there for the
// generation that the transaction publishes.
struct HeldMapEvent {
  mln_runtime_event_type type{};
  mln_runtime_event_payload_type payload_type = MLN_RUNTIME_EVENT_PAYLOAD_NONE;
  mln_runtime_event_payload payload{};
  uint32_t code = 0;
  std::string message;
  // The text that the payload's views point at; see
  // delivery::bind_event_strings().
  std::vector<std::string> strings;
};

// A camera command whose transitions ended with `outcome`, waiting for the
// generation of the snapshot that shows where they left the camera.
struct EndedCameraTransition {
  std::shared_ptr<CameraTransitionEnd> end;
  mln_camera_transition_outcome outcome =
    MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED;
};

// The camera transitions that the C API is cancelling, so that their finish
// callbacks report MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED.
struct CameraTransitionCancellation {
  bool active = false;
  // Every transition, or only the commands that carry `transition_id`.
  bool all = false;
  uint64_t transition_id = 0;
};

// One map's event state, shared by its observer and camera transitions.
struct MapEventState {
  // Any thread reads the mask; map commands write it on the runtime worker.
  std::atomic<uint64_t> mask;
  // Shared runtime event registration. Producers retire on the runtime
  // worker before map destruction.
  std::shared_ptr<RuntimeEventState> runtime_events;

  // Every field below is runtime-worker only, and only the map that owns this
  // state calls publish() or queues events through it.

  // The style setters clear this before a load and read it after, so a
  // subscription mask can never change their return status.
  std::string style_load_failure;
  bool style_load_failed = false;
  mln_map map = MLN_HANDLE_NULL;
  std::weak_ptr<MapObject> owner;
  uint64_t published_generation = 0;
  // Open map transactions. A transaction holds the events that the map raises
  // until it publishes, and transactions never nest.
  uint32_t transaction_depth = 0;
  std::vector<HeldMapEvent> held;
  // Camera commands with an end handler whose transitions are still running,
  // so that a map close can report them closed.
  std::vector<std::shared_ptr<CameraTransitionEnd>> running_transition_ends;
  // Camera commands whose transitions ended, waiting for the
  // MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE that MapLibre raises right after
  // a finish. A transaction takes every end that its work produced.
  std::vector<EndedCameraTransition> ended_transitions;
  CameraTransitionCancellation cancelling_transitions;

  [[nodiscard]] auto in_transaction() const noexcept -> bool {
    return transaction_depth != 0;
  }
  // Publishes a snapshot of the owning map and returns its generation, or
  // zero when that map is gone or being destroyed. Defined with the map.
  auto publish() -> uint64_t;
  // The generation for an event that reports a change in map state. Outside a
  // transaction this publishes first, so the snapshot includes the change.
  // Inside one it publishes nothing, because the transaction stamps its held
  // events when it publishes. A failed publish returns zero, which drops the
  // event, so no exception reaches the MapLibre callback that raised it.
  auto fresh_generation() noexcept -> uint64_t {
    if (in_transaction()) return published_generation;
    try {
      return publish();
    } catch (...) {
      return 0;
    }
  }
};

struct RuntimeEventRegistration;

struct RuntimeEventState {
  std::atomic<uint64_t> mask;
  // Written by runtime commands; offline producers hold their observer lock.
  std::shared_ptr<RuntimeEventRegistration> registration;
};

static_assert(
  std::atomic<uint64_t>::is_always_lock_free,
  "a lock-based 64-bit atomic would put a lock on the event "
  "producer hot path"
);
static_assert(
  MLN_RUNTIME_EVENT_FLAG_ALL == (MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS |
                                 MLN_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS),
  "every event type needs a mask bit in one of the two groups"
);
static_assert(
  MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR < 64,
  "an event type value past 63 needs a wider subscription mask"
);
// runtime.h promises that a later payload member widens the event stride
// without moving any field, which holds only while the payload is last.
static_assert(
  offsetof(mln_runtime_event, payload) + sizeof(mln_runtime_event_payload) ==
    sizeof(mln_runtime_event),
  "the payload must stay the last member of mln_runtime_event"
);

// One relaxed load, one shift, one test, without a handle-table or queue lock.
// Relaxed ordering is correct because the mask is policy rather than a
// synchronisation edge: the documented semantics gate events queued after a
// setter returns and say nothing about events already in flight.
[[nodiscard]] inline auto event_selected(
  const std::atomic<uint64_t>& mask, mln_runtime_event_type type
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
    void* user_data, mln_user_data_release release
  ) noexcept
      : user_data_(user_data), release_(release) {}

  RuntimeCallbackContext(const RuntimeCallbackContext&) = delete;
  auto operator=(const RuntimeCallbackContext&)
    -> RuntimeCallbackContext& = delete;

  ~RuntimeCallbackContext() noexcept {
    if (
      !owned_.load(std::memory_order_acquire) || release_ == nullptr ||
      process_exiting()
    ) {
      return;
    }
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
  mln_user_data_release release_ = nullptr;
  std::atomic_bool owned_{false};
};

struct RuntimeEventRegistration {
  mln_runtime_event_callback callback = nullptr;
  std::shared_ptr<RuntimeCallbackContext> context;
  delivery::QueuedRegistration* queued = nullptr;

  auto push(
    mln_runtime_event event, const std::string& message,
    std::vector<std::string> strings = {}
  ) const -> void;
};

struct ResourceProvider {
  mln_resource_provider_callback callback = nullptr;
  std::shared_ptr<RuntimeCallbackContext> context;
  // The requests that reach the provider. Null or empty claims every request.
  std::shared_ptr<const RouteTable> routes;
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
// replacement or clear command swaps the registration and completes; this lease
// keeps the retired registration and its user_data alive until the last
// in-flight callback returns. The lease retains the state, so it stays readable
// after the runtime handle is retired.
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
  // Whether the provider's routes claim a request. A request they do not
  // claim passes through to native loading.
  [[nodiscard]] auto matches(
    mln_resource_kind kind, std::string_view resolved_url
  ) const noexcept -> bool {
    return provider_.routes == nullptr ||
           provider_.routes->matches(kind, resolved_url);
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
  std::unordered_set<mln_offline_region_id> observed_regions;
};

struct RuntimeObject {
  mln_runtime self = MLN_HANDLE_NULL;
  void* platform_context = nullptr;
  ControlState control;
  RetirementTask disposal_task;
  std::shared_ptr<RuntimeObject> disposal_owner;
  // Reported by the shared finalizer after joining the worker; null for
  // dispose.
  std::shared_ptr<Completion> release_completion;
  std::atomic_bool disposal_requested = false;
  RuntimeExecutor executor;
  std::string asset_path;
  std::string cache_path;
  std::shared_ptr<mln::DatabaseFileSource> database_source;
  std::shared_ptr<mln::core::ResourceProviderState> resource_provider_state;
  std::shared_ptr<mln::core::OfflineRegionEventState> offline_event_state;
  std::shared_ptr<mln::core::ResourceTransformState> resource_transform_state;
  std::shared_ptr<mln::core::HttpHeaderTransformState>
    http_header_transform_state;
  std::shared_ptr<mln::core::RuntimeEventState> event_state;
};

template <>
struct HandleTraits<RuntimeObject> {
  static constexpr auto kind = HandleKind::Runtime;
  static constexpr auto leasable = true;
};

auto create_runtime(
  const mln_runtime_options* options, mln_runtime* out_runtime
) -> mln_status;
auto dispose_runtime(mln_runtime runtime) -> mln_status;
auto release_runtime(mln_runtime runtime, const mln_completion* completion)
  -> mln_status;
auto set_runtime_event_handler(
  mln_runtime runtime, const mln_runtime_event_handler* handler,
  const mln_completion* completion
) -> mln_status;
auto clear_runtime_event_handler(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status;
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
  mln_http_header_transform_response* response, mln_buffer_view name,
  mln_buffer_view value
) -> mln_status;
auto clear_http_header_transform(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status;
auto invoke_http_header_transform(
  void* platform_context, mln_resource_kind kind, const char* url
) noexcept -> HttpHeaders;
// Answers finished with the transformed URL, now or once the host answers.
auto invoke_resource_transform(
  void* platform_context, mln_resource_kind kind, const std::string& url,
  mln::ResourceTransform::FinishedCallback finished
) -> void;
auto run_ambient_cache_operation_start(
  mln_runtime runtime, mln_ambient_cache_operation operation,
  const mln_completion* completion
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
  mln_runtime runtime, mln_buffer_view side_database_path,
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
  mln_offline_region_download_state state;
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
auto retain_runtime_map(mln_runtime runtime) -> mln_status;
auto release_runtime_map(mln_runtime runtime) noexcept -> void;
[[nodiscard]] auto lease_runtime(mln_runtime runtime)
  -> std::shared_ptr<RuntimeObject>;
// A command with a completion moves its submission lease into the
// completion, which holds it until delivery and release. A discarded command
// holds it until its work has run.
struct RuntimeCommandTask : ExecutorTask {
  ControlLease lease;
  Completion* completion = nullptr;
  std::shared_ptr<Completion> retained_completion;
  void (*work)(RuntimeCommandTask&) = nullptr;
  void (*clear_work)(RuntimeCommandTask&) noexcept = nullptr;
};

auto run_runtime_command_task(RuntimeCommandTask& task) noexcept -> void;
auto submit_runtime_command_task(
  const std::shared_ptr<RuntimeObject>& runtime, RuntimeCommandTask& task
) -> mln_status;

template <typename Function>
auto submit_runtime_command(
  const std::shared_ptr<RuntimeObject>& runtime, Function function,
  const std::shared_ptr<Completion>& completion
) -> mln_status {
  struct Task final : RuntimeCommandTask {
    explicit Task(Function function) : function(std::move(function)) {
      command = true;
      run = [](ExecutorTask* base) noexcept {
        auto owner = std::unique_ptr<Task>{static_cast<Task*>(base)};
        run_runtime_command_task(*owner);
      };
      work = [](RuntimeCommandTask& base) {
        auto& self = static_cast<Task&>(base);
        std::invoke(std::move(*self.function));
      };
      clear_work = [](RuntimeCommandTask& base) noexcept {
        static_cast<Task&>(base).function.reset();
      };
    }
    std::optional<Function> function;
  };
  auto task = std::make_unique<Task>(std::move(function));
  task->retained_completion = completion;
  task->completion = completion.get();
  const auto status = submit_runtime_command_task(runtime, *task);
  if (status == MLN_STATUS_OK) static_cast<void>(task.release());
  return status;
}
auto submit_runtime_operation(
  const std::shared_ptr<RuntimeObject>& runtime,
  const std::shared_ptr<OperationObject>& operation,
  std::function<void()> function, const std::shared_ptr<Completion>& completion
) -> mln_status;

auto runtime_run_loop(RuntimeObject* runtime) -> mln::util::RunLoop&;

// Reads the options from the runtime object rather than its handle, so work
// accepted before the runtime's release still opens the runtime's own cache
// after the release has retired the handle.
auto resource_options_for_runtime(const RuntimeObject& runtime)
  -> mln::ResourceOptions;
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
// Map producers queue through queue_map_event() in the map module, which
// holds a transaction's events and stamps every event's generation. Producers
// test their subscription mask before they build an event.
auto push_runtime_map_event_payload(
  const std::shared_ptr<RuntimeEventState>& state, mln_map map,
  uint64_t generation, mln_runtime_event_type type,
  mln_runtime_event_payload_type payload_type,
  const mln_runtime_event_payload& payload, uint32_t code = 0,
  std::string message = {}, std::vector<std::string> strings = {}
) -> void;
auto push_runtime_map_events(
  const std::shared_ptr<RuntimeEventState>& state, mln_map map,
  uint64_t generation, std::vector<HeldMapEvent>& events
) -> void;

}  // namespace mln::core
