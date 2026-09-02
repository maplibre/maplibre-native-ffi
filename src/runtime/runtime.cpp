#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/storage/database_file_source.hpp>
#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/file_source_manager.hpp>
#include <mbgl/storage/offline.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/storage/sqlite3.hpp>
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/expected.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/run_loop.hpp>

#include "runtime/runtime.hpp"

#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

struct OfflineRegionData {
  mln_offline_region_id id = 0;
  uint32_t definition_type = 0;
  std::string style_url;
  mln_lat_lng_bounds bounds{};
  double min_zoom = 0.0;
  double max_zoom = 0.0;
  float pixel_ratio = 0.0F;
  bool include_ideographs = false;
  std::string geometry;
  std::vector<uint8_t> metadata;
};

namespace mln::core {

struct OfflineRegionSnapshotObject {
  OfflineRegionData data;
};

struct OfflineRegionListObject {
  std::vector<OfflineRegionData> regions;
};

template <>
struct HandleTraits<OfflineRegionSnapshotObject> {
  static constexpr auto kind = HandleKind::OfflineRegionSnapshot;
  static constexpr auto leasable = false;
};

template <>
struct HandleTraits<OfflineRegionListObject> {
  static constexpr auto kind = HandleKind::OfflineRegionList;
  static constexpr auto leasable = false;
};

}  // namespace mln::core

namespace mln::core {

// Holds its own reference to the wake state, so the state stays readable for a
// signal that races runtime teardown. Hosts destroy the two in either order.
struct WakeSourceObject {
  explicit WakeSourceObject(std::shared_ptr<WakeState> state_)
      : state(std::move(state_)) {}

  std::shared_ptr<WakeState> state;
};

// Signalling is an any-thread call that can race a destroy, so a lease keeps
// the wake state readable for the duration of the call.
template <>
struct HandleTraits<WakeSourceObject> {
  static constexpr auto kind = HandleKind::WakeSource;
  static constexpr auto leasable = true;
};

using OfflineOperationResult = std::variant<
  std::monostate, OfflineRegionData, std::optional<OfflineRegionData>,
  std::vector<OfflineRegionData>, mln_offline_region_status>;

struct OfflineOperation {
  mln_offline_operation_id id = 0;
  uint32_t kind = 0;
  uint32_t result_kind = MLN_OFFLINE_OPERATION_RESULT_NONE;
  bool completed = false;
  int32_t result_status = MLN_STATUS_OK;
  bool found = false;
  std::string message;
  OfflineOperationResult result;
};

struct OfflineOperationEventState {
  std::mutex mutex;
  mln::core::RuntimeObject* runtime = nullptr;
  bool alive = false;
  mln_offline_operation_id next_id = 1;
  std::unordered_map<mln_offline_operation_id, OfflineOperation> operations;
};

}  // namespace mln::core

namespace {

// Each mask constant must be the bit of the event type it selects, written
// against the shift rather than a literal, so an event type added without its
// mask constant fails the build here instead of silently queueing nothing.
#define MLN_ASSERT_EVENT_MASK_BIT(name)                                   \
  static_assert(                                                          \
    MLN_RUNTIME_EVENT_MASK_##name == 1ULL << MLN_RUNTIME_EVENT_##name,    \
    "MLN_RUNTIME_EVENT_MASK_" #name " must be the bit for its event type" \
  )

MLN_ASSERT_EVENT_MASK_BIT(MAP_CAMERA_WILL_CHANGE);
MLN_ASSERT_EVENT_MASK_BIT(MAP_CAMERA_IS_CHANGING);
MLN_ASSERT_EVENT_MASK_BIT(MAP_CAMERA_DID_CHANGE);
MLN_ASSERT_EVENT_MASK_BIT(MAP_STYLE_LOADED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_LOADING_STARTED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_LOADING_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_LOADING_FAILED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_IDLE);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_UPDATE_AVAILABLE);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_ERROR);
MLN_ASSERT_EVENT_MASK_BIT(MAP_STILL_IMAGE_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_STILL_IMAGE_FAILED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_FRAME_STARTED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_FRAME_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_MAP_STARTED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_MAP_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_STYLE_IMAGE_MISSING);
MLN_ASSERT_EVENT_MASK_BIT(MAP_TILE_ACTION);
MLN_ASSERT_EVENT_MASK_BIT(MAP_CAMERA_TRANSITION_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(OFFLINE_REGION_STATUS_CHANGED);
MLN_ASSERT_EVENT_MASK_BIT(OFFLINE_REGION_RESPONSE_ERROR);
MLN_ASSERT_EVENT_MASK_BIT(OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED);
MLN_ASSERT_EVENT_MASK_BIT(OFFLINE_OPERATION_COMPLETED);

#undef MLN_ASSERT_EVENT_MASK_BIT

// Maps the pointer-width token mbgl carries as its opaque platform context back
// to a runtime handle, which is 64 bits and does not fit in a void* on every
// target. Tokens are never reused: a recycled token would let a context that
// outlived its runtime name a later runtime. Liveness stays the handle table's
// to prove.
auto platform_context_registry_mutex() -> std::mutex& {
  static std::mutex value;
  return value;
}

auto platform_context_registry()
  -> std::unordered_map<std::uintptr_t, mln_runtime>& {
  static std::unordered_map<std::uintptr_t, mln_runtime> value;
  return value;
}

// Reserves a token for a runtime that is not published yet. This allocation can
// fail, so it runs before the handle table insert and a caller never sees a
// handle it owns reported as a failure.
auto reserve_platform_context() -> void* {
  static auto next_token = std::uintptr_t{1};
  const std::scoped_lock lock(platform_context_registry_mutex());
  const auto token = next_token++;
  platform_context_registry().emplace(token, MLN_HANDLE_NULL);
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<void*>(token);
}

auto bind_platform_context(void* platform_context, mln_runtime runtime) noexcept
  -> void {
  const std::scoped_lock lock(platform_context_registry_mutex());
  auto& registry = platform_context_registry();
  const auto found =
    registry.find(reinterpret_cast<std::uintptr_t>(platform_context));
  if (found != registry.end()) {
    found->second = runtime;
  }
}

auto unregister_platform_context(void* platform_context) noexcept -> void {
  const std::scoped_lock lock(platform_context_registry_mutex());
  platform_context_registry().erase(
    reinterpret_cast<std::uintptr_t>(platform_context)
  );
}

auto runtime_from_platform_context(void* platform_context) noexcept
  -> mln_runtime {
  const std::scoped_lock lock(platform_context_registry_mutex());
  const auto& registry = platform_context_registry();
  const auto found =
    registry.find(reinterpret_cast<std::uintptr_t>(platform_context));
  return found == registry.end() ? MLN_HANDLE_NULL : found->second;
}

auto live_runtime_threads_mutex() -> std::mutex& {
  static std::mutex value;
  return value;
}

// Mirrors the owner thread of every live runtime, so runtime creation can
// reject a thread that already owns one. The handle table does not iterate.
auto live_runtime_threads() -> std::unordered_set<std::thread::id>& {
  static std::unordered_set<std::thread::id> value;
  return value;
}

auto owner_thread_has_live_runtime(std::thread::id owner_thread) -> bool {
  const std::scoped_lock lock(live_runtime_threads_mutex());
  return live_runtime_threads().contains(owner_thread);
}

// Leases the resource transform registration for a MapLibre-owned thread. Only
// a reference count increment happens under the handle table lock; the caller
// takes the returned state's lock after this returns, so a writer waiting on
// that lock delays this runtime alone.
auto lease_resource_transform_state(void* platform_context) noexcept
  -> std::shared_ptr<mln::core::ResourceTransformState> {
  if (platform_context == nullptr) {
    return nullptr;
  }

  // try_resolve, not resolve: this runs on MapLibre worker and network threads,
  // where writing a diagnostic would clobber the entry point on the same stack.
  auto& table = mln::core::handle_table<mln::core::RuntimeObject>();
  const std::scoped_lock registry_lock(table.mutex());
  const auto* runtime =
    table.try_resolve_locked(runtime_from_platform_context(platform_context));
  if (runtime == nullptr) {
    return nullptr;
  }
  return runtime->resource_transform_state;
}

auto lease_http_header_transform_state(void* platform_context) noexcept
  -> std::shared_ptr<mln::core::HttpHeaderTransformState> {
  if (platform_context == nullptr) {
    return nullptr;
  }

  auto& table = mln::core::handle_table<mln::core::RuntimeObject>();
  const std::scoped_lock registry_lock(table.mutex());
  const auto* runtime =
    table.try_resolve_locked(runtime_from_platform_context(platform_context));
  if (runtime == nullptr) {
    return nullptr;
  }
  return runtime->http_header_transform_state;
}

// Leases the resource provider registration under the same locking rule as
// `lease_resource_transform_state()`.
auto lease_resource_provider_state(void* platform_context) noexcept
  -> std::shared_ptr<mln::core::ResourceProviderState> {
  if (platform_context == nullptr) {
    return nullptr;
  }

  auto& table = mln::core::handle_table<mln::core::RuntimeObject>();
  const std::scoped_lock registry_lock(table.mutex());
  const auto* runtime =
    table.try_resolve_locked(runtime_from_platform_context(platform_context));
  if (runtime == nullptr) {
    return nullptr;
  }
  return runtime->resource_provider_state;
}

// Reports why a take-result call has no result. This is the only route to a
// failed operation's text for a host that leaves
// MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED unselected.
auto set_offline_result_unavailable_error(
  const mln::core::OfflineOperation& operation
) -> void {
  if (operation.completed && !operation.message.empty()) {
    mln::core::set_thread_error(operation.message.c_str());
    return;
  }
  mln::core::set_thread_error("offline operation result is not available");
}

// A batch locates a message by a uint32_t offset and size, so a message is
// clamped at push rather than at drain time. The arena starts each drain empty,
// so the first event always fits and a bounded drain always makes progress.
auto truncated_event_message(std::string message) -> std::string {
  constexpr auto max_size = static_cast<size_t>(UINT32_MAX) - 1;
  if (message.size() > max_size) {
    message.resize(max_size);
  }
  return message;
}

auto valid_coordinate(const mln_lat_lng& coordinate) -> bool {
  return std::isfinite(coordinate.latitude) && coordinate.latitude >= -90.0 &&
         coordinate.latitude <= 90.0 && std::isfinite(coordinate.longitude);
}

auto validate_tile_pyramid_definition(
  const mln_offline_tile_pyramid_region_definition& definition
) -> mln_status {
  if (definition.size < sizeof(mln_offline_tile_pyramid_region_definition)) {
    mln::core::set_thread_error(
      "mln_offline_tile_pyramid_region_definition.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (definition.style_url == nullptr) {
    mln::core::set_thread_error("offline region style_url must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    !valid_coordinate(definition.bounds.southwest) ||
    !valid_coordinate(definition.bounds.northeast) ||
    definition.bounds.southwest.latitude >
      definition.bounds.northeast.latitude ||
    definition.bounds.southwest.longitude >
      definition.bounds.northeast.longitude
  ) {
    mln::core::set_thread_error("offline region bounds are invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    !std::isfinite(definition.min_zoom) || definition.min_zoom < 0.0 ||
    std::isnan(definition.max_zoom) || definition.max_zoom < definition.min_zoom
  ) {
    mln::core::set_thread_error("offline region zoom range is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!std::isfinite(definition.pixel_ratio) || definition.pixel_ratio < 0.0F) {
    mln::core::set_thread_error("offline region pixel_ratio is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_geometry_definition(
  const mln_offline_geometry_region_definition& definition
) -> mln_status {
  if (definition.size < sizeof(mln_offline_geometry_region_definition)) {
    mln::core::set_thread_error(
      "mln_offline_geometry_region_definition.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (definition.style_url == nullptr) {
    mln::core::set_thread_error("offline region style_url must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    !std::isfinite(definition.min_zoom) || definition.min_zoom < 0.0 ||
    std::isnan(definition.max_zoom) || definition.max_zoom < definition.min_zoom
  ) {
    mln::core::set_thread_error("offline region zoom range is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!std::isfinite(definition.pixel_ratio) || definition.pixel_ratio < 0.0F) {
    mln::core::set_thread_error("offline region pixel_ratio is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto geometry = mln::core::to_native_geometry(definition.geometry);
  if (!geometry) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (mln::core::geometry_lat_lngs(*geometry).empty()) {
    mln::core::set_thread_error(
      "offline region geometry must contain at least one coordinate"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_offline_region_definition(
  const mln_offline_region_definition* definition
) -> mln_status {
  if (definition == nullptr) {
    mln::core::set_thread_error("offline region definition must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (definition->size < sizeof(mln_offline_region_definition)) {
    mln::core::set_thread_error(
      "mln_offline_region_definition.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  switch (definition->type) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID:
      return validate_tile_pyramid_definition(definition->data.tile_pyramid);
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY:
      return validate_geometry_definition(definition->data.geometry);
    default:
      mln::core::set_thread_error("offline region definition type is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
}

auto to_native_offline_region_definition(
  const mln_offline_region_definition& definition
) -> mln::OfflineRegionDefinition {
  switch (definition.type) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID: {
      const auto& tile = definition.data.tile_pyramid;
      auto bounds = mln::LatLngBounds::hull(
        {tile.bounds.southwest.latitude, tile.bounds.southwest.longitude},
        {tile.bounds.northeast.latitude, tile.bounds.northeast.longitude}
      );
      return mln::OfflineTilePyramidRegionDefinition{
        std::string{tile.style_url},
        bounds,
        tile.min_zoom,
        tile.max_zoom,
        tile.pixel_ratio,
        tile.include_ideographs
      };
    }
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY: {
      const auto& geometry = definition.data.geometry;
      auto native_geometry = mln::core::to_native_geometry(geometry.geometry);
      if (!native_geometry) {
        throw std::logic_error(
          "offline geometry definition failed after validation"
        );
      }
      return mln::OfflineGeometryRegionDefinition{
        std::string{geometry.style_url},
        std::move(native_geometry.value()),
        geometry.min_zoom,
        geometry.max_zoom,
        geometry.pixel_ratio,
        geometry.include_ideographs
      };
    }
    default:
      throw std::logic_error(
        "offline region definition type failed after validation"
      );
  }
}

auto to_c_download_state(mln::OfflineRegionDownloadState state) -> uint32_t {
  switch (state) {
    case mln::OfflineRegionDownloadState::Inactive:
      return MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE;
    case mln::OfflineRegionDownloadState::Active:
      return MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE;
  }
  assert(false);
  return MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE;
}

auto to_c_status(const mln::OfflineRegionStatus& status)
  -> mln_offline_region_status {
  return mln_offline_region_status{
    .size = sizeof(mln_offline_region_status),
    .download_state = to_c_download_state(status.downloadState),
    .completed_resource_count = status.completedResourceCount,
    .completed_resource_size = status.completedResourceSize,
    .completed_tile_count = status.completedTileCount,
    .required_tile_count = status.requiredTileCount,
    .completed_tile_size = status.completedTileSize,
    .required_resource_count = status.requiredResourceCount,
    .required_resource_count_is_precise = status.requiredResourceCountIsPrecise,
    .complete = status.complete()
  };
}

auto to_native_download_state(uint32_t state)
  -> std::optional<mln::OfflineRegionDownloadState> {
  switch (state) {
    case MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE:
      return mln::OfflineRegionDownloadState::Inactive;
    case MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE:
      return mln::OfflineRegionDownloadState::Active;
    default:
      return std::nullopt;
  }
}

auto to_c_resource_error_reason(mln::Response::Error::Reason reason)
  -> uint32_t {
  switch (reason) {
    case mln::Response::Error::Reason::Success:
      return MLN_RESOURCE_ERROR_REASON_NONE;
    case mln::Response::Error::Reason::NotFound:
      return MLN_RESOURCE_ERROR_REASON_NOT_FOUND;
    case mln::Response::Error::Reason::Server:
      return MLN_RESOURCE_ERROR_REASON_SERVER;
    case mln::Response::Error::Reason::Connection:
      return MLN_RESOURCE_ERROR_REASON_CONNECTION;
    case mln::Response::Error::Reason::RateLimit:
      return MLN_RESOURCE_ERROR_REASON_RATE_LIMIT;
    case mln::Response::Error::Reason::Other:
      return MLN_RESOURCE_ERROR_REASON_OTHER;
  }
  assert(false);
  return MLN_RESOURCE_ERROR_REASON_OTHER;
}

auto to_c_region_data(const mln::OfflineRegion& region)
  -> std::optional<OfflineRegionData> {
  auto data = OfflineRegionData{
    .id = region.getID(),
    .definition_type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .style_url = {},
    .bounds = {},
    .min_zoom = 0.0,
    .max_zoom = 0.0,
    .pixel_ratio = 0.0F,
    .include_ideographs = false,
    .geometry = {},
    .metadata = region.getMetadata()
  };

  if (
    const auto* tile = std::get_if<mln::OfflineTilePyramidRegionDefinition>(
      &region.getDefinition()
    )
  ) {
    data.style_url = tile->styleURL;
    data.bounds = mln_lat_lng_bounds{
      .southwest =
        mln_lat_lng{
          .latitude = tile->bounds.south(), .longitude = tile->bounds.west()
        },
      .northeast = mln_lat_lng{
        .latitude = tile->bounds.north(), .longitude = tile->bounds.east()
      }
    };
    data.min_zoom = tile->minZoom;
    data.max_zoom = tile->maxZoom;
    data.pixel_ratio = tile->pixelRatio;
    data.include_ideographs = tile->includeIdeographs;
    return data;
  }

  if (
    const auto* geometry =
      std::get_if<mln::OfflineGeometryRegionDefinition>(&region.getDefinition())
  ) {
    data.definition_type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
    data.style_url = geometry->styleURL;
    data.min_zoom = geometry->minZoom;
    data.max_zoom = geometry->maxZoom;
    data.pixel_ratio = geometry->pixelRatio;
    data.include_ideographs = geometry->includeIdeographs;
    data.geometry =
      mln::core::serialize_geojson(mln::GeoJSON{geometry->geometry});
    return data;
  }

  mln::core::set_thread_error("offline region definition type is unsupported");
  return std::nullopt;
}

auto fill_region_info(
  const OfflineRegionData& data, mln_offline_region_info* out_info
) -> mln_status {
  if (out_info == nullptr || out_info->size < sizeof(mln_offline_region_info)) {
    mln::core::set_thread_error(
      "out_info must not be null and must have a valid size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto definition = mln_offline_region_definition{
    .size = sizeof(mln_offline_region_definition),
    .type = data.definition_type,
    .data = {}
  };
  switch (data.definition_type) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID:
      definition.data.tile_pyramid = mln_offline_tile_pyramid_region_definition{
        .size = sizeof(mln_offline_tile_pyramid_region_definition),
        .style_url = data.style_url.c_str(),
        .bounds = data.bounds,
        .min_zoom = data.min_zoom,
        .max_zoom = data.max_zoom,
        .pixel_ratio = data.pixel_ratio,
        .include_ideographs = data.include_ideographs
      };
      break;
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY:
      if (data.geometry.empty()) {
        mln::core::set_thread_error("offline region geometry is missing");
        return MLN_STATUS_NATIVE_ERROR;
      }
      definition.data.geometry = mln_offline_geometry_region_definition{
        .size = sizeof(mln_offline_geometry_region_definition),
        .style_url = data.style_url.c_str(),
        .geometry =
          {.data = data.geometry.data(), .size = data.geometry.size()},
        .min_zoom = data.min_zoom,
        .max_zoom = data.max_zoom,
        .pixel_ratio = data.pixel_ratio,
        .include_ideographs = data.include_ideographs
      };
      break;
    default:
      mln::core::set_thread_error("offline region definition type is invalid");
      return MLN_STATUS_NATIVE_ERROR;
  }

  *out_info = mln_offline_region_info{
    .size = sizeof(mln_offline_region_info),
    .id = data.id,
    .definition = definition,
    .metadata = data.metadata.empty() ? nullptr : data.metadata.data(),
    .metadata_size = data.metadata.size()
  };
  return MLN_STATUS_OK;
}

// Reports whether an offline region event would reach the queue: the region
// must be observed and the runtime's mask must select the type. The three
// observer callbacks call this before they build anything.
// `push_offline_region_event()` checks the region again, because the two run at
// different times and a region can be unobserved in between.
auto region_event_selected(
  const std::shared_ptr<mln::core::OfflineRegionEventState>& state,
  mln_offline_region_id region_id, uint32_t type
) -> bool {
  const std::scoped_lock state_lock(state->mutex);
  auto* runtime = state->runtime;
  if (!state->alive || runtime == nullptr) {
    return false;
  }
  if (!mln::core::event_selected(runtime->event_state->mask, type)) {
    return false;
  }
  const std::scoped_lock event_lock(runtime->event_mutex);
  return runtime->observed_offline_regions.contains(region_id);
}

auto push_offline_region_event(
  const std::shared_ptr<mln::core::OfflineRegionEventState>& state,
  mln_offline_region_id region_id, uint32_t type, uint32_t payload_type,
  const mln_runtime_event_payload& payload, std::string message = {}
) -> void {
  const std::scoped_lock state_lock(state->mutex);
  auto* runtime = state->runtime;
  if (!state->alive || runtime == nullptr) {
    return;
  }
  if (!mln::core::event_selected(runtime->event_state->mask, type)) {
    return;
  }

  auto event = mln::core::QueuedRuntimeEvent{
    .type = type,
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .source = runtime->self,
    .code = 0,
    .payload_type = payload_type,
    .payload = payload,
    .message = truncated_event_message(std::move(message)),
    .has_offline_region = true,
    .offline_region_id = region_id
  };

  {
    const std::scoped_lock event_lock(runtime->event_mutex);
    if (!runtime->observed_offline_regions.contains(region_id)) {
      return;
    }
    runtime->events.push_back(std::move(event));
  }
  // The offline region observer runs off the owner-thread run loop, so this
  // signal releases a parked owner thread.
  mln::core::signal_wake(runtime->wake_state);
}

class OfflineRegionRuntimeObserver final : public mln::OfflineRegionObserver {
 public:
  OfflineRegionRuntimeObserver(
    std::shared_ptr<mln::core::OfflineRegionEventState> state,
    mln_offline_region_id region_id
  )
      : state_(std::move(state)), region_id_(region_id) {}

  void statusChanged(mln::OfflineRegionStatus status) override {
    if (!region_event_selected(
          state_, region_id_, MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED
        )) {
      return;
    }
    auto payload = mln::core::zeroed_event_payload();
    payload.offline_region_status = mln_runtime_event_offline_region_status{
      .region_id = region_id_, .status = to_c_status(status)
    };
    push_offline_region_event(
      state_, region_id_, MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS, payload
    );
  }

  void responseError(mln::Response::Error error) override {
    if (!region_event_selected(
          state_, region_id_, MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR
        )) {
      return;
    }
    auto payload = mln::core::zeroed_event_payload();
    payload.offline_region_response_error =
      mln_runtime_event_offline_region_response_error{
        .region_id = region_id_,
        .reason = to_c_resource_error_reason(error.reason)
      };
    push_offline_region_event(
      state_, region_id_, MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR, payload,
      error.message
    );
  }

  void mapboxTileCountLimitExceeded(uint64_t limit) override {
    if (!region_event_selected(
          state_, region_id_,
          MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED
        )) {
      return;
    }
    auto payload = mln::core::zeroed_event_payload();
    payload.offline_region_tile_count_limit =
      mln_runtime_event_offline_region_tile_count_limit{
        .region_id = region_id_, .limit = limit
      };
    push_offline_region_event(
      state_, region_id_,
      MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
      MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT, payload
    );
  }

 private:
  std::shared_ptr<mln::core::OfflineRegionEventState> state_;
  mln_offline_region_id region_id_;
};

auto set_offline_region_observed_flag(
  mln::core::RuntimeObject* runtime, mln_offline_region_id region_id,
  bool observed
) -> void {
  const std::scoped_lock lock(runtime->event_mutex);
  if (observed) {
    runtime->observed_offline_regions.insert(region_id);
  } else {
    runtime->observed_offline_regions.erase(region_id);
    std::erase_if(runtime->events, [region_id](const auto& event) -> bool {
      return event.has_offline_region && event.offline_region_id == region_id;
    });
  }
}

// Fills the object before it is published, so a throwing fill leaves no handle
// behind.
template <typename Fill>
auto register_offline_region_snapshot_from_result(Fill&& fill)
  -> mln_offline_region_snapshot {
  auto owned = std::make_shared<mln::core::OfflineRegionSnapshotObject>();
  std::forward<Fill>(fill)(owned->data);
  return mln::core::handle_table<mln::core::OfflineRegionSnapshotObject>()
    .insert(std::move(owned));
}

template <typename Fill>
auto register_offline_region_list_from_result(Fill&& fill)
  -> mln_offline_region_list {
  auto owned = std::make_shared<mln::core::OfflineRegionListObject>();
  std::forward<Fill>(fill)(owned->regions);
  return mln::core::handle_table<mln::core::OfflineRegionListObject>().insert(
    std::move(owned)
  );
}

auto to_c_region_data_list(const mln::OfflineRegions& native_regions)
  -> std::optional<std::vector<OfflineRegionData>> {
  auto regions = std::vector<OfflineRegionData>{};
  regions.reserve(native_regions.size());
  for (const auto& region : native_regions) {
    auto data = to_c_region_data(region);
    if (!data) {
      return std::nullopt;
    }
    regions.push_back(std::move(*data));
  }
  return regions;
}

auto exception_message(std::exception_ptr exception, const char* fallback)
  -> std::string {
  if (!exception) {
    return {};
  }
  try {
    std::rethrow_exception(exception);
  } catch (const std::exception& caught) {
    return caught.what();
  } catch (...) {
    return fallback;
  }
}

auto validate_offline_operation_output(
  mln_runtime runtime, mln::core::RuntimeObject*& out_runtime,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  const auto status = mln::core::validate_runtime(runtime, out_runtime);
  if (status != MLN_STATUS_OK) {
    if (out_operation_id != nullptr) {
      *out_operation_id = 0;
    }
    return status;
  }
  if (out_operation_id == nullptr) {
    mln::core::set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_operation_id = 0;
  return MLN_STATUS_OK;
}

auto register_offline_operation(
  mln_runtime runtime_handle, uint32_t kind, uint32_t result_kind,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  mln::core::RuntimeObject* runtime = nullptr;
  const auto status = validate_offline_operation_output(
    runtime_handle, runtime, out_operation_id
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }

  const std::scoped_lock lock(runtime->offline_operation_state->mutex);
  auto& state = *runtime->offline_operation_state;
  if (!state.alive || state.runtime != runtime || state.next_id == 0) {
    mln::core::set_thread_error("offline operation state is unavailable");
    return MLN_STATUS_INVALID_STATE;
  }
  const auto id = state.next_id++;
  state.operations.emplace(
    id, mln::core::OfflineOperation{
          .id = id,
          .kind = kind,
          .result_kind = result_kind,
          .completed = false,
          .result_status = MLN_STATUS_OK,
          .found = false,
          .message = {},
          .result = std::monostate{}
        }
  );
  *out_operation_id = id;
  return MLN_STATUS_OK;
}

auto erase_queued_offline_operation_events(
  mln::core::RuntimeObject* runtime, mln_offline_operation_id operation_id
) -> void {
  const std::scoped_lock event_lock(runtime->event_mutex);
  std::erase_if(runtime->events, [operation_id](const auto& event) -> bool {
    return event.has_offline_operation &&
           event.offline_operation_id == operation_id;
  });
}

auto erase_offline_operation_registration(
  mln::core::RuntimeObject* runtime, mln_offline_operation_id operation_id
) -> void {
  if (runtime == nullptr || operation_id == 0) {
    return;
  }
  auto state = runtime->offline_operation_state;
  if (!state) {
    return;
  }
  {
    const std::scoped_lock state_lock(state->mutex);
    state->operations.erase(operation_id);
  }

  erase_queued_offline_operation_events(runtime, operation_id);
}

template <typename Schedule>
auto schedule_registered_offline_operation(
  mln::core::RuntimeObject* runtime, uint32_t kind, uint32_t result_kind,
  mln_offline_operation_id* out_operation_id, Schedule&& schedule
) -> mln_status {
  auto register_status = register_offline_operation(
    runtime->self, kind, result_kind, out_operation_id
  );
  if (register_status != MLN_STATUS_OK) {
    return register_status;
  }
  const auto operation_id = *out_operation_id;
  auto state = runtime->offline_operation_state;
  try {
    std::forward<Schedule>(schedule)(state, operation_id);
  } catch (...) {
    erase_offline_operation_registration(runtime, operation_id);
    *out_operation_id = 0;
    throw;
  }
  return MLN_STATUS_OK;
}

auto make_offline_completion_payload(
  const mln::core::OfflineOperation& operation
) -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.offline_operation_completed =
    mln_runtime_event_offline_operation_completed{
      .operation_id = operation.id,
      .operation_kind = operation.kind,
      .result_kind = operation.result_kind,
      .result_status = operation.result_status,
      .found = operation.found
    };
  return payload;
}

auto complete_offline_operation(
  const std::shared_ptr<mln::core::OfflineOperationEventState>& state,
  mln_offline_operation_id operation_id, int32_t result_status,
  mln::core::OfflineOperationResult result, bool found = false,
  std::string message = {}
) -> void {
  const std::scoped_lock state_lock(state->mutex);
  auto* runtime = state->runtime;
  if (!state->alive || runtime == nullptr) {
    return;
  }

  auto found_operation = state->operations.find(operation_id);
  if (found_operation == state->operations.end()) {
    return;
  }

  // The result and its failure text are what a take-result call reports, so
  // they are recorded whatever the mask selects. Only the event is gated.
  auto& operation = found_operation->second;
  operation.completed = true;
  operation.result_status = result_status;
  operation.found = found;
  operation.message = std::move(message);
  operation.result = std::move(result);

  if (!mln::core::event_selected(
        runtime->event_state->mask,
        MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED
      )) {
    return;
  }

  auto event = mln::core::QueuedRuntimeEvent{
    .type = MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED,
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .source = runtime->self,
    .code = result_status,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED,
    .payload = make_offline_completion_payload(operation),
    .message = truncated_event_message(operation.message),
    .has_offline_region = false,
    .offline_region_id = 0,
    .has_offline_operation = true,
    .offline_operation_id = operation_id
  };

  {
    const std::scoped_lock event_lock(runtime->event_mutex);
    runtime->events.push_back(std::move(event));
  }
  mln::core::signal_wake(runtime->wake_state);
}

auto complete_offline_operation_error(
  const std::shared_ptr<mln::core::OfflineOperationEventState>& state,
  mln_offline_operation_id operation_id, int32_t status, std::string message
) -> void {
  complete_offline_operation(
    state, operation_id, status, std::monostate{}, false, std::move(message)
  );
}

auto complete_from_exception(
  const std::shared_ptr<mln::core::OfflineOperationEventState>& state,
  mln_offline_operation_id operation_id, std::exception_ptr exception,
  const char* fallback
) -> void {
  if (exception) {
    complete_offline_operation_error(
      state, operation_id, MLN_STATUS_NATIVE_ERROR,
      exception_message(exception, fallback)
    );
    return;
  }
  complete_offline_operation(
    state, operation_id, MLN_STATUS_OK, std::monostate{}
  );
}

auto validate_runtime_options(const mln_runtime_options* options)
  -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }

  if (options->size < sizeof(mln_runtime_options)) {
    mln::core::set_thread_error("mln_runtime_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->flags != 0U) {
    mln::core::set_thread_error(
      "mln_runtime_options.flags contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // Validated here as well as in the setter, so a mask this library cannot
  // honour is rejected wherever it arrives.
  constexpr auto known_mask_bits =
    static_cast<uint64_t>(MLN_RUNTIME_EVENT_MASK_ALL);
  if ((options->event_mask & ~known_mask_bits) != 0U) {
    mln::core::set_thread_error(
      "mln_runtime_options.event_mask contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

}  // namespace

namespace mln::core {

namespace {

auto database_source_for_runtime(RuntimeObject* runtime)
  -> std::shared_ptr<mln::DatabaseFileSource> {
  if (runtime->database_source != nullptr) {
    return runtime->database_source;
  }

  auto source = mln::FileSourceManager::get()->getFileSource(
    mln::FileSourceType::Database, resource_options_for_runtime(runtime->self),
    mln::ClientOptions()
  );
  // MapLibre is built without RTTI. The registered FileSourceType::Database
  // factory always returns a DatabaseFileSource.
  auto database = std::static_pointer_cast<mln::DatabaseFileSource>(source);
  runtime->database_source = database;
  return runtime->database_source;
}

}  // namespace

// Only the owner thread destroys a runtime, so the borrowed object stays alive
// for as long as the calling thread can use it.
auto validate_runtime(mln_runtime runtime, RuntimeObject*& out_runtime)
  -> mln_status {
  out_runtime = handle_table<RuntimeObject>().resolve(runtime);
  if (out_runtime == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (out_runtime->owner_thread != std::this_thread::get_id()) {
    set_thread_error("runtime call must be made on its owner thread");
    return MLN_STATUS_WRONG_THREAD;
  }

  return MLN_STATUS_OK;
}

auto create_runtime(
  const mln_runtime_options* options, mln_runtime* out_runtime
) -> mln_status {
  const auto options_status = validate_runtime_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  if (out_runtime == nullptr) {
    set_thread_error("out_runtime must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (*out_runtime != MLN_HANDLE_NULL) {
    set_thread_error("out_runtime must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto owner_thread = std::this_thread::get_id();
  if (owner_thread_has_live_runtime(owner_thread)) {
    set_thread_error("owner thread already has a live runtime");
    return MLN_STATUS_INVALID_STATE;
  }
  if (mln::Scheduler::GetCurrent(false) != nullptr) {
    set_thread_error("owner thread already has an active MapLibre scheduler");
    return MLN_STATUS_INVALID_STATE;
  }

  auto owned_runtime = std::make_shared<RuntimeObject>();
  owned_runtime->owner_thread = owner_thread;
  owned_runtime->wake_state = std::make_shared<WakeState>();
  // The mask cell exists before the run loop, so every producer this runtime
  // can reach reads a live cell.
  owned_runtime->event_state = std::make_shared<RuntimeEventState>();
  // Null options keep the library's own default, which is every event type.
  owned_runtime->event_state->mask.store(
    options != nullptr ? options->event_mask
                       : static_cast<uint64_t>(MLN_RUNTIME_EVENT_MASK_ALL),
    std::memory_order_relaxed
  );
  owned_runtime->run_loop =
    std::make_unique<mln::util::RunLoop>(mln::util::RunLoop::Type::New);
  // `setPlatformCallback` is an unlocked assignment, so it happens while the
  // run loop is reachable only from this thread.
  owned_runtime->run_loop->setPlatformCallback(
    [state = owned_runtime->wake_state]() -> void { signal_wake(state); }
  );
  // The gate runs on the owner thread inside the drain, and the runtime owns
  // the run loop, so the raw pointer cannot outlive it.
  owned_runtime->run_loop->setProcessGate(
    [live = owned_runtime.get()]() -> bool {
      if (!live->pump_deadline.has_value()) {
        return true;
      }
      if (!live->pump_ran_task) {
        live->pump_ran_task = true;
        return true;
      }
      if (std::chrono::steady_clock::now() < *live->pump_deadline) {
        return true;
      }
      live->pump_budget_exhausted = true;
      // Denying disarms the gate: a task inside the drain can wait for this
      // same run loop to empty, and that nested drain must run to completion
      // rather than spin against an expired budget.
      live->pump_deadline.reset();
      return false;
    }
  );
  owned_runtime->asset_path =
    options == nullptr || options->asset_path == nullptr
      ? std::string{}
      : std::string{options->asset_path};
  owned_runtime->cache_path =
    options == nullptr || options->cache_path == nullptr
      ? std::string{}
      : std::string{options->cache_path};
  owned_runtime->offline_event_state =
    std::make_shared<OfflineRegionEventState>();
  owned_runtime->offline_event_state->runtime = owned_runtime.get();
  owned_runtime->offline_event_state->alive = true;
  owned_runtime->offline_operation_state =
    std::make_shared<OfflineOperationEventState>();
  owned_runtime->offline_operation_state->runtime = owned_runtime.get();
  owned_runtime->offline_operation_state->alive = true;
  owned_runtime->resource_transform_state =
    std::make_shared<ResourceTransformState>();
  owned_runtime->http_header_transform_state =
    std::make_shared<HttpHeaderTransformState>();
  owned_runtime->resource_provider_state =
    std::make_shared<ResourceProviderState>();
  auto* published = owned_runtime.get();
  // Reserving the token allocates, so it happens before this thread is marked
  // as owning a runtime. The failure path below reads the local, not the
  // object: the insert takes the shared_ptr by value, so a throw there destroys
  // the runtime while unwinding.
  auto* const platform_context = reserve_platform_context();
  published->platform_context = platform_context;
  {
    const std::scoped_lock lock(live_runtime_threads_mutex());
    live_runtime_threads().insert(owner_thread);
  }
  try {
    *out_runtime =
      handle_table<RuntimeObject>().insert(std::move(owned_runtime));
  } catch (...) {
    // The caller receives no handle, so it has no way to give the owner thread
    // back. Releasing it here keeps a failed creation from rejecting every
    // later one on the same thread.
    {
      const std::scoped_lock lock(live_runtime_threads_mutex());
      live_runtime_threads().erase(owner_thread);
    }
    unregister_platform_context(platform_context);
    throw;
  }
  published->self = *out_runtime;
  bind_platform_context(platform_context, *out_runtime);
  return MLN_STATUS_OK;
}

auto set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (transform == nullptr) {
    set_thread_error("resource transform must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (transform->size < sizeof(mln_resource_transform)) {
    set_thread_error("mln_resource_transform.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (transform->callback == nullptr) {
    set_thread_error("resource transform callback must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto& state = *live->resource_transform_state;
  const std::unique_lock lock(state.mutex);
  state.callback = transform->callback;
  state.user_data = transform->user_data;
  return MLN_STATUS_OK;
}

auto resource_transform_response_set_url(
  mln_resource_transform_response* response, const char* url, size_t url_size
) -> mln_status {
  if (response == nullptr) {
    set_thread_error("resource transform response must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (response->size < sizeof(mln_resource_transform_response)) {
    set_thread_error("mln_resource_transform_response.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url == nullptr && url_size != 0) {
    set_thread_error("resource transform response URL must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (url_size != 0 && std::memchr(url, '\0', url_size) != nullptr) {
    set_thread_error("resource transform response URL contains embedded NUL");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto* const replacement_url = static_cast<std::string*>(response->context);
  if (replacement_url == nullptr) {
    set_thread_error(
      "resource transform response URL can only be set during a transform "
      "callback"
    );
    return MLN_STATUS_INVALID_STATE;
  }
  if (url_size == 0) {
    replacement_url->clear();
    response->url = nullptr;
    return MLN_STATUS_OK;
  }

  replacement_url->assign(url, url_size);
  response->url = replacement_url->c_str();
  return MLN_STATUS_OK;
}

auto clear_resource_transform(mln_runtime runtime) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  auto& state = *live->resource_transform_state;
  const std::unique_lock lock(state.mutex);
  state.callback = nullptr;
  state.user_data = nullptr;
  return MLN_STATUS_OK;
}

auto set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform
) -> mln_status {
  RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (transform == nullptr) {
    set_thread_error("HTTP header transform must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (transform->size < sizeof(mln_http_header_transform)) {
    set_thread_error("mln_http_header_transform.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (transform->callback == nullptr) {
    set_thread_error("HTTP header transform callback must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

#if defined(OHOS_PLATFORM)
  set_thread_error(
    "HTTP header transforms are unsupported on OpenHarmony because its HTTP "
    "client cannot prevent transformed headers from crossing origins"
  );
  return MLN_STATUS_UNSUPPORTED;
#elif defined(__EMSCRIPTEN__)
  // fetch() with redirect: "manual" is no help either: a cross-origin manual
  // redirect is an opaque response whose Location cannot be read.
  set_thread_error(
    "HTTP header transforms are unsupported in the browser because "
    "emscripten_fetch follows redirects itself and cannot drop transformed "
    "headers when the origin changes; serve the request with "
    "mln_runtime_set_resource_provider() instead"
  );
  return MLN_STATUS_UNSUPPORTED;
#endif

  auto& state = *live->http_header_transform_state;
  const std::unique_lock lock(state.mutex);
  state.callback = transform->callback;
  state.user_data = transform->user_data;
  return MLN_STATUS_OK;
}

namespace {

auto http_header_names_equal(std::string_view left, std::string_view right)
  -> bool {
  const auto ascii_lower = [](unsigned char character) -> unsigned char {
    return character >= 'A' && character <= 'Z'
             ? static_cast<unsigned char>(character + ('a' - 'A'))
             : character;
  };
  return std::ranges::equal(
    left, right, [&](char left_character, char right_character) -> bool {
      return ascii_lower(static_cast<unsigned char>(left_character)) ==
             ascii_lower(static_cast<unsigned char>(right_character));
    }
  );
}

auto is_valid_utf8(const char* value, std::size_t size) -> bool {
  const auto bytes = reinterpret_cast<const unsigned char*>(value);
  std::size_t index = 0;
  const auto continuation = [&](std::size_t offset) -> bool {
    return index + offset < size && bytes[index + offset] >= 0x80U &&
           bytes[index + offset] <= 0xBFU;
  };
  while (index < size) {
    const auto first = bytes[index];
    if (first <= 0x7FU) {
      ++index;
      continue;
    }
    if (first >= 0xC2U && first <= 0xDFU && continuation(1)) {
      index += 2;
      continue;
    }
    if (
      first == 0xE0U && index + 2 < size && bytes[index + 1] >= 0xA0U &&
      bytes[index + 1] <= 0xBFU && continuation(2)
    ) {
      index += 3;
      continue;
    }
    if (
      ((first >= 0xE1U && first <= 0xECU) ||
       (first >= 0xEEU && first <= 0xEFU)) &&
      continuation(1) && continuation(2)
    ) {
      index += 3;
      continue;
    }
    if (
      first == 0xEDU && index + 2 < size && bytes[index + 1] >= 0x80U &&
      bytes[index + 1] <= 0x9FU && continuation(2)
    ) {
      index += 3;
      continue;
    }
    if (
      first == 0xF0U && index + 3 < size && bytes[index + 1] >= 0x90U &&
      bytes[index + 1] <= 0xBFU && continuation(2) && continuation(3)
    ) {
      index += 4;
      continue;
    }
    if (
      first >= 0xF1U && first <= 0xF3U && continuation(1) && continuation(2) &&
      continuation(3)
    ) {
      index += 4;
      continue;
    }
    if (
      first == 0xF4U && index + 3 < size && bytes[index + 1] >= 0x80U &&
      bytes[index + 1] <= 0x8FU && continuation(2) && continuation(3)
    ) {
      index += 4;
      continue;
    }
    return false;
  }
  return true;
}

}  // namespace

auto validate_http_header(
  const char* name, size_t name_size, const char* value, size_t value_size
) -> mln_status {
  if (name == nullptr && name_size != 0) {
    set_thread_error("HTTP header name must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (value == nullptr && value_size != 0) {
    set_thread_error("HTTP header value must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto is_field_name_character = [](unsigned char character) -> bool {
    return (character >= '0' && character <= '9') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') || character == '!' ||
           character == '#' || character == '$' || character == '%' ||
           character == '&' || character == '\'' || character == '*' ||
           character == '+' || character == '-' || character == '.' ||
           character == '^' || character == '_' || character == '`' ||
           character == '|' || character == '~';
  };
  if (
    name_size == 0 ||
    !std::all_of(name, name + name_size, [&](char character) -> bool {
      return is_field_name_character(static_cast<unsigned char>(character));
    })
  ) {
    set_thread_error("HTTP header name is not a valid field name");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (
    value_size != 0 &&
    !std::all_of(value, value + value_size, [](char character) -> bool {
      const auto byte = static_cast<unsigned char>(character);
      return byte == '\t' || (byte >= 0x20U && byte != 0x7FU);
    })
  ) {
    set_thread_error("HTTP header value contains a disallowed control byte");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (value_size != 0 && !is_valid_utf8(value, value_size)) {
    set_thread_error("HTTP header value must be valid UTF-8");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto name_view = std::string_view{name, name_size};
  constexpr auto ManagedHeaders = std::array<std::string_view, 15>{
    "Host",
    "Content-Length",
    "Transfer-Encoding",
    "Connection",
    "Proxy-Connection",
    "Proxy-Authorization",
    "Keep-Alive",
    "TE",
    "Trailer",
    "Upgrade",
    "Range",
    "If-None-Match",
    "If-Modified-Since",
    "Accept-Encoding",
    "User-Agent",
  };
  const auto managed = std::ranges::find_if(
    ManagedHeaders, [&](std::string_view candidate) -> bool {
      return http_header_names_equal(name_view, candidate);
    }
  );
  if (managed != ManagedHeaders.end()) {
    const auto diagnostic =
      "HTTP header \"" + std::string{name_view} +
      "\" is managed by MapLibre or the platform transport";
    set_thread_error(diagnostic.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto http_header_transform_response_set(
  mln_http_header_transform_response* response, const char* name,
  size_t name_size, const char* value, size_t value_size
) -> mln_status {
  if (response == nullptr) {
    set_thread_error("HTTP header transform response must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (response->size < sizeof(mln_http_header_transform_response)) {
    set_thread_error("mln_http_header_transform_response.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto validation =
    validate_http_header(name, name_size, value, value_size);
  if (validation != MLN_STATUS_OK) {
    return validation;
  }
  const auto name_view = std::string_view{name, name_size};

  auto* const headers = static_cast<HttpHeaders*>(response->context);
  if (headers == nullptr) {
    set_thread_error(
      "HTTP headers can only be set during an HTTP header transform callback"
    );
    return MLN_STATUS_INVALID_STATE;
  }

  auto existing =
    std::ranges::find_if(*headers, [&](const HttpHeader& header) -> bool {
      return http_header_names_equal(header.first, name_view);
    });
  auto copied = HttpHeader{
    std::string{name_view},
    value_size == 0 ? std::string{} : std::string{value, value_size},
  };
  if (existing == headers->end()) {
    headers->push_back(std::move(copied));
  } else {
    *existing = std::move(copied);
  }
  return MLN_STATUS_OK;
}

auto clear_http_header_transform(mln_runtime runtime) -> mln_status {
  RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  auto& state = *live->http_header_transform_state;
  const std::unique_lock lock(state.mutex);
  state.callback = nullptr;
  state.user_data = nullptr;
  return MLN_STATUS_OK;
}

auto run_ambient_cache_operation_start(
  mln_runtime runtime, uint32_t operation,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  switch (operation) {
    case MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE:
    case MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE:
    case MLN_AMBIENT_CACHE_OPERATION_INVALIDATE:
    case MLN_AMBIENT_CACHE_OPERATION_CLEAR:
      break;
    default:
      set_thread_error("ambient cache operation is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_AMBIENT_CACHE,
    MLN_OFFLINE_OPERATION_RESULT_NONE, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      auto callback = [state,
                       operation_id](std::exception_ptr exception) -> void {
        complete_from_exception(
          state, operation_id, exception, "ambient cache operation failed"
        );
      };

      switch (operation) {
        case MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE:
          database->resetDatabase(std::move(callback));
          break;
        case MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE:
          database->packDatabase(std::move(callback));
          break;
        case MLN_AMBIENT_CACHE_OPERATION_INVALIDATE:
          database->invalidateAmbientCache(std::move(callback));
          break;
        case MLN_AMBIENT_CACHE_OPERATION_CLEAR:
          database->clearAmbientCache(std::move(callback));
          break;
        default:
          throw std::logic_error(
            "ambient cache operation failed after validation"
          );
      }
    }
  );
}

auto set_maximum_ambient_cache_size_start(
  mln_runtime runtime, std::uint64_t size,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_SET_MAXIMUM_AMBIENT_CACHE_SIZE,
    MLN_OFFLINE_OPERATION_RESULT_NONE, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->setMaximumAmbientCacheSize(
        size, [state, operation_id](std::exception_ptr exception) -> void {
          complete_from_exception(
            state, operation_id, exception,
            "setting the maximum ambient cache size failed"
          );
        }
      );
    }
  );
}

auto offline_operation_discard(
  mln_runtime runtime, mln_offline_operation_id operation_id
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (operation_id == 0) {
    set_thread_error("offline operation ID is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto state = live->offline_operation_state;
  const std::scoped_lock state_lock(state->mutex);
  auto found = state->operations.find(operation_id);
  if (found == state->operations.end()) {
    set_thread_error("offline operation is unknown");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  state->operations.erase(found);

  erase_queued_offline_operation_events(live, operation_id);
  return MLN_STATUS_OK;
}

auto offline_region_create_start(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  const auto definition_status = validate_offline_region_definition(definition);
  if (definition_status != MLN_STATUS_OK) {
    return definition_status;
  }
  if (metadata == nullptr && metadata_size != 0) {
    set_thread_error("offline region metadata must not be null when non-empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  const auto native_definition =
    to_native_offline_region_definition(*definition);
  auto native_metadata = mln::OfflineRegionMetadata{};
  if (metadata_size != 0) {
    native_metadata.resize(metadata_size);
    std::memcpy(native_metadata.data(), metadata, metadata_size);
  }
  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_CREATE,
    MLN_OFFLINE_OPERATION_RESULT_REGION, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->createOfflineRegion(
        native_definition, native_metadata,
        [state, operation_id](
          mln::expected<mln::OfflineRegion, std::exception_ptr> result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(
                result.error(), "offline region creation failed"
              )
            );
            return;
          }
          auto data = to_c_region_data(result.value());
          if (!data) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(
            state, operation_id, MLN_STATUS_OK, std::move(*data), true
          );
        }
      );
    }
  );
}

auto offline_region_get_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_GET,
    MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->getOfflineRegion(
        region_id,
        [state, operation_id](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation(
              state, operation_id, MLN_STATUS_OK,
              std::optional<OfflineRegionData>{}, false
            );
            return;
          }
          auto data = to_c_region_data(region.value());
          if (!data) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(
            state, operation_id, MLN_STATUS_OK, std::move(data), true
          );
        }
      );
    }
  );
}

auto offline_regions_list_start(
  mln_runtime runtime, mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGIONS_LIST,
    MLN_OFFLINE_OPERATION_RESULT_REGION_LIST, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->listOfflineRegions(
        [state, operation_id](
          mln::expected<mln::OfflineRegions, std::exception_ptr> result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region list failed")
            );
            return;
          }
          auto regions = to_c_region_data_list(result.value());
          if (!regions) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(
            state, operation_id, MLN_STATUS_OK, std::move(*regions), true
          );
        }
      );
    }
  );
}

auto offline_regions_merge_database_start(
  mln_runtime runtime, const char* side_database_path,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (side_database_path == nullptr) {
    set_thread_error("side_database_path must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto path = std::string{side_database_path};
  const auto side_database =
    mapbox::sqlite::Database::tryOpen(path, mapbox::sqlite::ReadOnly);
  if (std::holds_alternative<mapbox::sqlite::Exception>(side_database)) {
    set_thread_error(
      "side database path must identify an existing readable database file"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE,
    MLN_OFFLINE_OPERATION_RESULT_REGION_LIST, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->mergeOfflineRegions(
        path,
        [state, operation_id](
          mln::expected<mln::OfflineRegions, std::exception_ptr> result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(
                result.error(), "offline region database merge failed"
              )
            );
            return;
          }
          auto regions = to_c_region_data_list(result.value());
          if (!regions) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(
            state, operation_id, MLN_STATUS_OK, std::move(*regions), true
          );
        }
      );
    }
  );
}

auto offline_region_update_metadata_start(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (metadata == nullptr && metadata_size != 0) {
    set_thread_error("offline region metadata must not be null when non-empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  auto native_metadata = mln::OfflineRegionMetadata{};
  if (metadata_size != 0) {
    native_metadata.resize(metadata_size);
    std::memcpy(native_metadata.data(), metadata, metadata_size);
  }
  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA,
    MLN_OFFLINE_OPERATION_RESULT_REGION, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->updateOfflineMetadata(
        region_id, native_metadata,
        [database, state, operation_id, region_id](
          mln::expected<mln::OfflineRegionMetadata, std::exception_ptr> result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(
                result.error(), "offline region metadata update failed"
              )
            );
            return;
          }
          database->getOfflineRegion(
            region_id,
            [state, operation_id](
              mln::expected<
                std::optional<mln::OfflineRegion>, std::exception_ptr>
                result
            ) -> void {
              if (!result) {
                complete_offline_operation_error(
                  state, operation_id, MLN_STATUS_NATIVE_ERROR,
                  exception_message(result.error(), "offline region get failed")
                );
                return;
              }
              const auto& region = result.value();
              if (!region.has_value()) {
                complete_offline_operation_error(
                  state, operation_id, MLN_STATUS_INVALID_ARGUMENT,
                  "offline region not found"
                );
                return;
              }
              auto data = to_c_region_data(region.value());
              if (!data) {
                complete_offline_operation_error(
                  state, operation_id, MLN_STATUS_UNSUPPORTED,
                  "offline region definition type is unsupported"
                );
                return;
              }
              complete_offline_operation(
                state, operation_id, MLN_STATUS_OK, std::move(*data), true
              );
            }
          );
        }
      );
    }
  );
}

auto offline_region_get_status_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_GET_STATUS,
    MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, operation_id](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_INVALID_ARGUMENT,
              "offline region not found"
            );
            return;
          }
          database->getOfflineRegionStatus(
            region.value(),
            [state, operation_id](
              mln::expected<mln::OfflineRegionStatus, std::exception_ptr> result
            ) -> void {
              if (!result) {
                complete_offline_operation_error(
                  state, operation_id, MLN_STATUS_NATIVE_ERROR,
                  exception_message(
                    result.error(), "offline region status query failed"
                  )
                );
                return;
              }
              complete_offline_operation(
                state, operation_id, MLN_STATUS_OK, to_c_status(result.value()),
                true
              );
            }
          );
        }
      );
    }
  );
}

auto offline_region_set_observed_start(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED,
    MLN_OFFLINE_OPERATION_RESULT_NONE, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, operation_id, region_id, observed](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_INVALID_ARGUMENT,
              "offline region not found"
            );
            return;
          }
          auto event_state = std::shared_ptr<OfflineRegionEventState>{};
          {
            const std::scoped_lock state_lock(state->mutex);
            if (!state->alive || state->runtime == nullptr) {
              return;
            }
            set_offline_region_observed_flag(
              state->runtime, region_id, observed
            );
            event_state = state->runtime->offline_event_state;
          }
          auto observer = observed
                            ? std::make_unique<OfflineRegionRuntimeObserver>(
                                std::move(event_state), region_id
                              )
                            : nullptr;
          database->setOfflineRegionObserver(
            region.value(), std::move(observer)
          );
          complete_offline_operation(
            state, operation_id, MLN_STATUS_OK, std::monostate{}
          );
        }
      );
    }
  );
}

auto offline_region_set_download_state_start(
  mln_runtime runtime, OfflineRegionDownloadStateRequest request,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  const auto native_state = to_native_download_state(request.state);
  if (!native_state) {
    set_thread_error("offline region download state is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE,
    MLN_OFFLINE_OPERATION_RESULT_NONE, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->getOfflineRegion(
        request.region_id,
        [database, state, operation_id, native_state](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_INVALID_ARGUMENT,
              "offline region not found"
            );
            return;
          }
          database->setOfflineRegionDownloadState(
            region.value(), *native_state
          );
          complete_offline_operation(
            state, operation_id, MLN_STATUS_OK, std::monostate{}
          );
        }
      );
    }
  );
}

auto offline_region_invalidate_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_INVALIDATE,
    MLN_OFFLINE_OPERATION_RESULT_NONE, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, operation_id](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_INVALID_ARGUMENT,
              "offline region not found"
            );
            return;
          }
          database->invalidateOfflineRegion(
            region.value(),
            [state, operation_id](std::exception_ptr exception) -> void {
              complete_from_exception(
                state, operation_id, exception,
                "offline region invalidation failed"
              );
            }
          );
        }
      );
    }
  );
}

auto offline_region_delete_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) -> mln_status {
  if (out_operation_id != nullptr) {
    *out_operation_id = 0;
  }
  mln::core::RuntimeObject* live = nullptr;
  const auto runtime_status = validate_runtime(runtime, live);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  if (out_operation_id == nullptr) {
    set_thread_error("out_operation_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_DELETE,
    MLN_OFFLINE_OPERATION_RESULT_NONE, out_operation_id,
    [&](auto state, auto operation_id) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, operation_id, region_id](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, operation_id, MLN_STATUS_INVALID_ARGUMENT,
              "offline region not found"
            );
            return;
          }
          {
            const std::scoped_lock state_lock(state->mutex);
            if (!state->alive || state->runtime == nullptr) {
              return;
            }
            set_offline_region_observed_flag(state->runtime, region_id, false);
          }
          database->setOfflineRegionObserver(region.value(), nullptr);
          database->setOfflineRegionDownloadState(
            region.value(), mln::OfflineRegionDownloadState::Inactive
          );
          database->deleteOfflineRegion(
            region.value(),
            [state, operation_id](std::exception_ptr exception) -> void {
              complete_from_exception(
                state, operation_id, exception, "offline region deletion failed"
              );
            }
          );
        }
      );
    }
  );
}

auto offline_region_create_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_region == nullptr || *out_region != MLN_HANDLE_NULL) {
    set_thread_error("out_region must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = live->offline_operation_state;
  {
    const std::scoped_lock lock(state->mutex);
    auto found = state->operations.find(operation_id);
    if (found == state->operations.end()) {
      set_thread_error("offline operation is unknown");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto& operation = found->second;
    if (
      !operation.completed ||
      operation.kind != MLN_OFFLINE_OPERATION_REGION_CREATE ||
      operation.result_status != MLN_STATUS_OK
    ) {
      set_offline_result_unavailable_error(operation);
      return MLN_STATUS_INVALID_STATE;
    }
    auto* result = std::get_if<OfflineRegionData>(&operation.result);
    if (result == nullptr) {
      set_thread_error("offline operation result kind is invalid");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto snapshot = register_offline_region_snapshot_from_result(
    [state, operation_id](OfflineRegionData& data) -> void {
      const std::scoped_lock lock(state->mutex);
      auto found = state->operations.find(operation_id);
      if (found == state->operations.end()) {
        throw std::logic_error(
          "offline operation disappeared while taking result"
        );
      }
      auto* result = std::get_if<OfflineRegionData>(&found->second.result);
      if (result == nullptr) {
        throw std::logic_error(
          "offline operation result kind changed while taking result"
        );
      }
      data = std::move(*result);
    }
  );
  erase_offline_operation_registration(live, operation_id);
  *out_region = snapshot;
  return MLN_STATUS_OK;
}

auto offline_region_get_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region, bool* out_found
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_region == nullptr || *out_region != MLN_HANDLE_NULL ||
    out_found == nullptr
  ) {
    set_thread_error(
      "out_region must point to the null handle and out_found must not be null"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = live->offline_operation_state;
  auto has_region = false;
  {
    const std::scoped_lock lock(state->mutex);
    auto found = state->operations.find(operation_id);
    if (found == state->operations.end()) {
      set_thread_error("offline operation is unknown");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto& operation = found->second;
    if (
      !operation.completed ||
      operation.kind != MLN_OFFLINE_OPERATION_REGION_GET ||
      operation.result_status != MLN_STATUS_OK
    ) {
      set_offline_result_unavailable_error(operation);
      return MLN_STATUS_INVALID_STATE;
    }
    auto* result =
      std::get_if<std::optional<OfflineRegionData>>(&operation.result);
    if (result == nullptr) {
      set_thread_error("offline operation result kind is invalid");
      return MLN_STATUS_INVALID_STATE;
    }
    has_region = result->has_value();
  }
  if (!has_region) {
    erase_offline_operation_registration(live, operation_id);
    *out_found = false;
    return MLN_STATUS_OK;
  }
  auto snapshot = register_offline_region_snapshot_from_result(
    [state, operation_id](OfflineRegionData& data) -> void {
      const std::scoped_lock lock(state->mutex);
      auto found = state->operations.find(operation_id);
      if (found == state->operations.end()) {
        throw std::logic_error(
          "offline operation disappeared while taking result"
        );
      }
      auto* result =
        std::get_if<std::optional<OfflineRegionData>>(&found->second.result);
      if (result == nullptr || !result->has_value()) {
        throw std::logic_error(
          "offline operation result kind changed while taking result"
        );
      }
      data = std::move(**result);
    }
  );
  erase_offline_operation_registration(live, operation_id);
  *out_region = snapshot;
  *out_found = true;
  return MLN_STATUS_OK;
}

auto offline_regions_list_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list* out_regions
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_regions == nullptr || *out_regions != MLN_HANDLE_NULL) {
    set_thread_error("out_regions must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = live->offline_operation_state;
  {
    const std::scoped_lock lock(state->mutex);
    auto found = state->operations.find(operation_id);
    if (found == state->operations.end()) {
      set_thread_error("offline operation is unknown");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto& operation = found->second;
    if (
      !operation.completed ||
      operation.kind != MLN_OFFLINE_OPERATION_REGIONS_LIST ||
      operation.result_status != MLN_STATUS_OK
    ) {
      set_offline_result_unavailable_error(operation);
      return MLN_STATUS_INVALID_STATE;
    }
    auto* result =
      std::get_if<std::vector<OfflineRegionData>>(&operation.result);
    if (result == nullptr) {
      set_thread_error("offline operation result kind is invalid");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto regions = register_offline_region_list_from_result(
    [state, operation_id](std::vector<OfflineRegionData>& destination) -> void {
      const std::scoped_lock lock(state->mutex);
      auto found = state->operations.find(operation_id);
      if (found == state->operations.end()) {
        throw std::logic_error(
          "offline operation disappeared while taking result"
        );
      }
      auto* result =
        std::get_if<std::vector<OfflineRegionData>>(&found->second.result);
      if (result == nullptr) {
        throw std::logic_error(
          "offline operation result kind changed while taking result"
        );
      }
      destination = std::move(*result);
    }
  );
  erase_offline_operation_registration(live, operation_id);
  *out_regions = regions;
  return MLN_STATUS_OK;
}

auto offline_regions_merge_database_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list* out_regions
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_regions == nullptr || *out_regions != MLN_HANDLE_NULL) {
    set_thread_error("out_regions must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = live->offline_operation_state;
  {
    const std::scoped_lock lock(state->mutex);
    auto found = state->operations.find(operation_id);
    if (found == state->operations.end()) {
      set_thread_error("offline operation is unknown");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto& operation = found->second;
    if (
      !operation.completed ||
      operation.kind != MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE ||
      operation.result_status != MLN_STATUS_OK
    ) {
      set_offline_result_unavailable_error(operation);
      return MLN_STATUS_INVALID_STATE;
    }
    auto* result =
      std::get_if<std::vector<OfflineRegionData>>(&operation.result);
    if (result == nullptr) {
      set_thread_error("offline operation result kind is invalid");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto regions = register_offline_region_list_from_result(
    [state, operation_id](std::vector<OfflineRegionData>& destination) -> void {
      const std::scoped_lock lock(state->mutex);
      auto found = state->operations.find(operation_id);
      if (found == state->operations.end()) {
        throw std::logic_error(
          "offline operation disappeared while taking result"
        );
      }
      auto* result =
        std::get_if<std::vector<OfflineRegionData>>(&found->second.result);
      if (result == nullptr) {
        throw std::logic_error(
          "offline operation result kind changed while taking result"
        );
      }
      destination = std::move(*result);
    }
  );
  erase_offline_operation_registration(live, operation_id);
  *out_regions = regions;
  return MLN_STATUS_OK;
}

auto offline_region_update_metadata_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_region == nullptr || *out_region != MLN_HANDLE_NULL) {
    set_thread_error("out_region must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = live->offline_operation_state;
  {
    const std::scoped_lock lock(state->mutex);
    auto found = state->operations.find(operation_id);
    if (found == state->operations.end()) {
      set_thread_error("offline operation is unknown");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto& operation = found->second;
    if (
      !operation.completed ||
      operation.kind != MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA ||
      operation.result_status != MLN_STATUS_OK
    ) {
      set_offline_result_unavailable_error(operation);
      return MLN_STATUS_INVALID_STATE;
    }
    auto* result = std::get_if<OfflineRegionData>(&operation.result);
    if (result == nullptr) {
      set_thread_error("offline operation result kind is invalid");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  auto snapshot = register_offline_region_snapshot_from_result(
    [state, operation_id](OfflineRegionData& data) -> void {
      const std::scoped_lock lock(state->mutex);
      auto found = state->operations.find(operation_id);
      if (found == state->operations.end()) {
        throw std::logic_error(
          "offline operation disappeared while taking result"
        );
      }
      auto* result = std::get_if<OfflineRegionData>(&found->second.result);
      if (result == nullptr) {
        throw std::logic_error(
          "offline operation result kind changed while taking result"
        );
      }
      data = std::move(*result);
    }
  );
  erase_offline_operation_registration(live, operation_id);
  *out_region = snapshot;
  return MLN_STATUS_OK;
}

auto offline_region_get_status_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_status* out_status
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_status == nullptr ||
    out_status->size < sizeof(mln_offline_region_status)
  ) {
    set_thread_error("out_status must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = live->offline_operation_state;
  auto result_status = mln_offline_region_status{};
  {
    const std::scoped_lock lock(state->mutex);
    auto found = state->operations.find(operation_id);
    if (found == state->operations.end()) {
      set_thread_error("offline operation is unknown");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto& operation = found->second;
    if (
      !operation.completed ||
      operation.kind != MLN_OFFLINE_OPERATION_REGION_GET_STATUS ||
      operation.result_status != MLN_STATUS_OK
    ) {
      set_offline_result_unavailable_error(operation);
      return MLN_STATUS_INVALID_STATE;
    }
    auto* result = std::get_if<mln_offline_region_status>(&operation.result);
    if (result == nullptr) {
      set_thread_error("offline operation result kind is invalid");
      return MLN_STATUS_INVALID_STATE;
    }
    result_status = *result;
  }
  *out_status = result_status;
  erase_offline_operation_registration(live, operation_id);
  return MLN_STATUS_OK;
}

auto offline_region_snapshot_get(
  mln_offline_region_snapshot snapshot, mln_offline_region_info* out_info
) -> mln_status {
  // Offline snapshots and lists carry no thread affinity, so the lock spans the
  // read to keep another thread from destroying one mid-read.
  auto& table = handle_table<OfflineRegionSnapshotObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_snapshot = table.resolve_locked(snapshot);
  if (live_snapshot == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return fill_region_info(live_snapshot->data, out_info);
}

auto offline_region_snapshot_destroy(
  mln_offline_region_snapshot snapshot
) noexcept -> void {
  static_cast<void>(
    handle_table<OfflineRegionSnapshotObject>().remove(snapshot)
  );
}

auto offline_region_list_count(mln_offline_region_list list, size_t* out_count)
  -> mln_status {
  auto& table = handle_table<OfflineRegionListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_count == nullptr) {
    set_thread_error("out_count must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_count = live_list->regions.size();
  return MLN_STATUS_OK;
}

auto offline_region_list_get(
  mln_offline_region_list list, size_t index, mln_offline_region_info* out_info
) -> mln_status {
  auto& table = handle_table<OfflineRegionListObject>();
  const std::scoped_lock lock(table.mutex());
  const auto* live_list = table.resolve_locked(list);
  if (live_list == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (index >= live_list->regions.size()) {
    set_thread_error("offline region list index is out of range");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return fill_region_info(live_list->regions.at(index), out_info);
}

auto offline_region_list_destroy(mln_offline_region_list list) noexcept
  -> void {
  static_cast<void>(handle_table<OfflineRegionListObject>().remove(list));
}

auto set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (provider == nullptr) {
    set_thread_error("provider must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (provider->size < sizeof(mln_resource_provider)) {
    set_thread_error("mln_resource_provider.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (provider->callback == nullptr) {
    set_thread_error("provider callback must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // The exclusive provider lock waits for every callback that leased the
  // previous provider, so that callback and its `user_data` are unreferenced
  // once this returns.
  auto& state = *live->resource_provider_state;
  const std::unique_lock lock(state.mutex);
  state.registered = true;
  state.provider = ResourceProvider{
    .callback = provider->callback,
    .user_data = provider->user_data,
  };
  return MLN_STATUS_OK;
}

auto clear_resource_provider(mln_runtime runtime) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  auto& state = *live->resource_provider_state;
  const std::unique_lock lock(state.mutex);
  state.registered = false;
  state.provider = ResourceProvider{};
  return MLN_STATUS_OK;
}

auto destroy_runtime(mln_runtime runtime) -> mln_status {
  // Take ownership of the runtime out of the table, then release the
  // process-global table lock before any teardown step that can block on a
  // native callback. The waits below then stall this runtime alone.
  std::shared_ptr<RuntimeObject> owned_runtime;
  auto owner_thread = std::thread::id{};
  {
    auto& table = handle_table<RuntimeObject>();
    const std::scoped_lock lock(table.mutex());
    auto* live = table.resolve_locked(runtime);
    if (live == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }

    if (live->owner_thread != std::this_thread::get_id()) {
      set_thread_error("runtime must be destroyed on its owner thread");
      return MLN_STATUS_WRONG_THREAD;
    }

    if (live->live_maps != 0) {
      set_thread_error("runtime still owns live maps");
      return MLN_STATUS_INVALID_STATE;
    }

    owner_thread = live->owner_thread;
    owned_runtime = table.remove_locked(runtime);
  }
  {
    const std::scoped_lock lock(live_runtime_threads_mutex());
    live_runtime_threads().erase(owner_thread);
  }
  unregister_platform_context(owned_runtime->platform_context);

  // A resource transform callback that entered `invoke_resource_transform()`
  // before the erase above holds a shared transform lock, so this wait covers
  // every callback that can still observe the runtime. A lease that reaches the
  // shared lock afterwards reads the cleared registration and calls nothing.
  {
    auto& transform_state = *owned_runtime->resource_transform_state;
    const std::unique_lock transform_lock(transform_state.mutex);
    transform_state.callback = nullptr;
    transform_state.user_data = nullptr;
  }

  {
    auto& transform_state = *owned_runtime->http_header_transform_state;
    const std::unique_lock transform_lock(transform_state.mutex);
    transform_state.callback = nullptr;
    transform_state.user_data = nullptr;
  }

  // Same wait for a resource provider callback that leased the provider before
  // the erase above.
  {
    auto& provider_state = *owned_runtime->resource_provider_state;
    const std::unique_lock provider_lock(provider_state.mutex);
    provider_state.registered = false;
    provider_state.provider = ResourceProvider{};
  }

  {
    const std::scoped_lock state_lock(
      owned_runtime->offline_event_state->mutex
    );
    owned_runtime->offline_event_state->alive = false;
    owned_runtime->offline_event_state->runtime = nullptr;
    const std::scoped_lock event_lock(owned_runtime->event_mutex);
    owned_runtime->observed_offline_regions.clear();
    std::erase_if(owned_runtime->events, [](const auto& event) -> bool {
      return event.has_offline_region;
    });
  }

  {
    const std::scoped_lock state_lock(
      owned_runtime->offline_operation_state->mutex
    );
    owned_runtime->offline_operation_state->alive = false;
    owned_runtime->offline_operation_state->runtime = nullptr;
    owned_runtime->offline_operation_state->operations.clear();
    const std::scoped_lock event_lock(owned_runtime->event_mutex);
    std::erase_if(owned_runtime->events, [](const auto& event) -> bool {
      return event.has_offline_operation;
    });
  }

  // The last drained batch is freed with the runtime, the documented end of its
  // readable window.
  owned_runtime->event_drain_staging.clear();
  owned_runtime->event_batch_events.clear();
  owned_runtime->event_batch_messages.clear();

  // Retiring the wake state before the run loop is released covers a late
  // `mln_wake_source_signal()` and the run loop teardown's final iteration.
  {
    const std::scoped_lock wake_lock(owned_runtime->wake_state->mutex);
    owned_runtime->wake_state->alive = false;
    owned_runtime->wake_state->signaled = false;
  }

  // Releasing the run loop and the database file source can join native
  // threads, so it happens with the registry lock released.
  owned_runtime.reset();
  return MLN_STATUS_OK;
}

auto signal_wake(const std::shared_ptr<WakeState>& state) noexcept -> void {
  if (!state) {
    return;
  }
  {
    const std::scoped_lock lock(state->mutex);
    if (!state->alive) {
      return;
    }
    state->signaled = true;
  }
  // MapLibre calls this while holding the `RunLoop` mutex, so the notify
  // happens outside the wake lock to keep that path short.
  state->condition.notify_all();
}

auto pump_runtime(mln_runtime runtime, int64_t timeout_ms, int64_t budget_ms)
  -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  // Queued unread events return the call without parking, so a host that
  // stopped polling before the queue emptied keeps making progress.
  auto queued_events = false;
  {
    const std::scoped_lock event_lock(live->event_mutex);
    queued_events = !live->events.empty();
  }

  {
    auto& wake = *live->wake_state;
    std::unique_lock lock(wake.mutex);
    if (!queued_events && timeout_ms != 0 && !wake.signaled) {
      if (timeout_ms < 0) {
        wake.condition.wait(lock, [&wake]() -> bool { return wake.signaled; });
      } else {
        wake.condition.wait_for(
          lock, std::chrono::milliseconds{timeout_ms},
          [&wake]() -> bool { return wake.signaled; }
        );
      }
    }
    // The flag is cleared before the drain, so work arriving during the drain
    // leaves it set for the next pump.
    wake.signaled = false;
  }

  if (budget_ms >= 0) {
    // A budget past the clock's range would overflow the addition, so it
    // saturates to an unbounded drain.
    const auto now = std::chrono::steady_clock::now();
    const auto headroom = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::time_point::max() - now
    );
    live->pump_deadline = std::chrono::milliseconds{budget_ms} < headroom
                            ? now + std::chrono::milliseconds{budget_ms}
                            : std::chrono::steady_clock::time_point::max();
  } else {
    live->pump_deadline.reset();
  }
  live->pump_ran_task = false;
  live->pump_budget_exhausted = false;
  live->run_loop->runOnce();
  live->pump_deadline.reset();
  // The gate only denies while a task is queued, so an exhausted budget means
  // work remains.
  if (live->pump_budget_exhausted) {
    signal_wake(live->wake_state);
  }
  return MLN_STATUS_OK;
}

auto acquire_wake_source(mln_runtime runtime, mln_wake_source* out_source)
  -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_source == nullptr) {
    set_thread_error("out_source must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_source != MLN_HANDLE_NULL) {
    set_thread_error("out_source must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_source = handle_table<WakeSourceObject>().insert(
    std::make_shared<WakeSourceObject>(live->wake_state)
  );
  return MLN_STATUS_OK;
}

auto signal_wake_source(mln_wake_source source) -> mln_status {
  const auto live = handle_table<WakeSourceObject>().lease(source);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  signal_wake(live->state);
  return MLN_STATUS_OK;
}

auto destroy_wake_source(mln_wake_source source) noexcept -> void {
  static_cast<void>(handle_table<WakeSourceObject>().remove(source));
}

auto drain_runtime_events(
  mln_runtime runtime, size_t max_events, mln_runtime_event_batch* out_batch
) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_batch == nullptr || out_batch->size < sizeof(mln_runtime_event_batch)
  ) {
    set_thread_error("out_batch must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // Clearing first ends the previous batch's window, so an empty drain
  // invalidates it too. Capacity survives, so a steady-state drain allocates
  // nothing.
  auto& staging = live->event_drain_staging;
  auto& events = live->event_batch_events;
  auto& messages = live->event_batch_messages;
  staging.clear();
  events.clear();
  messages.clear();

  auto remaining_count = size_t{0};
  auto arena_size = size_t{0};
  auto drain_count = size_t{0};
  {
    const std::scoped_lock lock(live->event_mutex);
    for (const auto& queued : live->events) {
      if (max_events != 0 && drain_count >= max_events) {
        break;
      }
      // The first event always fits, so a bounded drain always makes progress.
      // An event without a message takes no arena bytes, not even a terminator.
      const auto& next_message = queued.message;
      const auto next_size =
        next_message.empty() ? size_t{0} : next_message.size() + 1;
      if (
        drain_count != 0 &&
        next_size > static_cast<size_t>(UINT32_MAX) - arena_size
      ) {
        break;
      }
      arena_size += next_size;
      drain_count += 1;
    }

    // Finish every allocation before removing an event. The reserved staging
    // moves and batch writes below cannot allocate, so a failed reservation
    // leaves the complete queue available to the next drain.
    static_assert(
      std::is_nothrow_move_constructible_v<mln::core::QueuedRuntimeEvent>
    );
    staging.reserve(drain_count);
    events.reserve(drain_count);
    messages.reserve(arena_size);
    for (auto index = size_t{0}; index < drain_count; index += 1) {
      staging.push_back(std::move(live->events.front()));
      live->events.pop_front();
    }
    remaining_count = live->events.size();
  }

  // The batch is built outside the lock, so the lock hold covers only the
  // queue moves above.
  for (const auto& staged : staging) {
    const auto message_size = static_cast<uint32_t>(staged.message.size());
    const auto message_offset = static_cast<uint32_t>(messages.size());
    events.push_back(
      mln_runtime_event{
        .type = staged.type,
        .source_type = staged.source_type,
        .source = staged.source,
        .code = staged.code,
        .payload_type = staged.payload_type,
        .message_offset = message_size == 0 ? 0 : message_offset,
        .message_size = message_size,
        .payload = staged.payload
      }
    );
    if (message_size != 0) {
      messages.append(staged.message);
      messages.push_back('\0');
    }
  }
  staging.clear();

  *out_batch = mln_runtime_event_batch{
    .size = sizeof(mln_runtime_event_batch),
    .event_size = sizeof(mln_runtime_event),
    .events = events.empty() ? nullptr : events.data(),
    .event_count = events.size(),
    .messages = messages.empty() ? nullptr : messages.data(),
    .messages_size = messages.size(),
    .remaining_count = remaining_count
  };
  return MLN_STATUS_OK;
}

auto set_runtime_event_mask(mln_runtime runtime, uint64_t mask) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if ((mask & ~static_cast<uint64_t>(MLN_RUNTIME_EVENT_MASK_ALL)) != 0U) {
    set_thread_error("mask contains unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // The whole value is stored, including the map-event bits this runtime's own
  // producers never test, so a getter reports what a host wrote.
  live->event_state->mask.store(mask, std::memory_order_relaxed);
  return MLN_STATUS_OK;
}

auto get_runtime_event_mask(mln_runtime runtime, uint64_t* out_mask)
  -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_mask == nullptr) {
    set_thread_error("out_mask must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_mask = live->event_state->mask.load(std::memory_order_relaxed);
  return MLN_STATUS_OK;
}

auto retain_runtime_map(mln_runtime runtime) -> mln_status {
  mln::core::RuntimeObject* live = nullptr;
  const auto status = validate_runtime(runtime, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  ++live->live_maps;
  return MLN_STATUS_OK;
}

auto release_runtime_map(mln_runtime runtime) noexcept -> void {
  auto& table = handle_table<RuntimeObject>();
  const std::scoped_lock lock(table.mutex());
  auto* live = table.try_resolve_locked(runtime);
  if (live != nullptr && live->live_maps != 0) {
    --live->live_maps;
  }
}

auto runtime_run_loop(RuntimeObject* runtime) -> mln::util::RunLoop& {
  return *runtime->run_loop;
}

auto resource_options_for_runtime(mln_runtime runtime) -> mln::ResourceOptions {
  auto options = mln::ResourceOptions::Default();
  const auto* live = handle_table<RuntimeObject>().try_resolve(runtime);
  if (live == nullptr) {
    return options;
  }
  options.withPlatformContext(live->platform_context);
  if (!live->asset_path.empty()) {
    options.withAssetPath(live->asset_path);
  }
  if (!live->cache_path.empty()) {
    options.withCachePath(live->cache_path);
  }
  return options;
}

auto acquire_resource_provider_for_platform_context(
  void* platform_context
) noexcept -> std::optional<ResourceProviderLease> {
  const auto state = lease_resource_provider_state(platform_context);
  if (state == nullptr) {
    return std::nullopt;
  }

  // The lease keeps this state readable, so the shared lock is taken with the
  // registry lock released. A callback that reaches the shared lock before
  // `destroy_runtime()` holds teardown until it returns; one that arrives later
  // finds an empty registration.
  std::shared_lock provider_lock(state->mutex);
  if (!state->registered) {
    return std::nullopt;
  }
  return ResourceProviderLease{
    state, std::move(provider_lock), state->provider
  };
}

auto has_resource_transform_for_platform_context(
  void* platform_context
) noexcept -> bool {
  const auto state = lease_resource_transform_state(platform_context);
  if (state == nullptr) {
    return false;
  }

  const std::shared_lock transform_lock(state->mutex);
  return state->callback != nullptr;
}

auto invoke_resource_transform(
  void* platform_context, uint32_t kind, const char* url,
  std::string& out_replacement_url
) noexcept -> mln_status {
  const auto state = lease_resource_transform_state(platform_context);
  if (state == nullptr) {
    return MLN_STATUS_OK;
  }

  // Same locking rule as `acquire_resource_provider_for_platform_context()`.
  const std::shared_lock transform_lock(state->mutex);
  const auto callback = state->callback;
  if (callback == nullptr) {
    return MLN_STATUS_OK;
  }

  auto response = mln_resource_transform_response{
    .size = sizeof(mln_resource_transform_response),
    .url = nullptr,
    .context = &out_replacement_url,
  };
  try {
    const auto status = callback(state->user_data, kind, url, &response);
    if (
      status == MLN_STATUS_OK && response.url != nullptr &&
      *response.url != '\0'
    ) {
      out_replacement_url = response.url;
    }
    return status;
  } catch (...) {
    return MLN_STATUS_NATIVE_ERROR;
  }
}

auto invoke_http_header_transform(
  void* platform_context, uint32_t kind, const char* url
) noexcept -> HttpHeaders {
  const auto state = lease_http_header_transform_state(platform_context);
  if (state == nullptr) {
    return {};
  }

  const std::shared_lock transform_lock(state->mutex);
  const auto callback = state->callback;
  if (callback == nullptr) {
    return {};
  }

  auto headers = HttpHeaders{};
  auto response = mln_http_header_transform_response{
    .size = sizeof(mln_http_header_transform_response),
    .context = &headers,
  };
  try {
    if (callback(state->user_data, kind, url, &response) != MLN_STATUS_OK) {
      return {};
    }
    return headers;
  } catch (...) {
    return {};
  }
}

auto push_runtime_map_event(
  mln_runtime runtime, mln_map map, uint32_t type, int32_t code,
  const char* message
) -> void {
  push_runtime_map_event_payload(
    runtime, map, type, MLN_RUNTIME_EVENT_PAYLOAD_NONE, zeroed_event_payload(),
    code, message == nullptr ? std::string{} : std::string{message}
  );
}

// Producers test their subscription mask before they call this, so it holds no
// mask test of its own.
auto push_runtime_map_event_payload(
  mln_runtime runtime, mln_map map, uint32_t type, uint32_t payload_type,
  const mln_runtime_event_payload& payload, int32_t code, std::string message
) -> void {
  // try_resolve, not resolve: this runs from map observer callbacks under a
  // pump, where writing a diagnostic would clobber the pump's own.
  auto* live = handle_table<RuntimeObject>().try_resolve(runtime);
  if (live == nullptr) {
    return;
  }

  auto event = QueuedRuntimeEvent{
    .type = type,
    .source_type = MLN_RUNTIME_EVENT_SOURCE_MAP,
    .source = map,
    .code = code,
    .payload_type = payload_type,
    .payload = payload,
    .message = truncated_event_message(std::move(message))
  };

  {
    const std::scoped_lock lock(live->event_mutex);
    if (map != MLN_HANDLE_NULL && !live->event_maps.contains(map)) {
      return;
    }
    // A render draws the latest update, so one unread render-update event
    // covers every invalidation queued behind it. Comparing against the tail
    // alone preserves the order of every other event.
    if (
      type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE &&
      map != MLN_HANDLE_NULL && !live->events.empty() &&
      live->events.back().type == type && live->events.back().source == map
    ) {
      return;
    }
    live->events.push_back(std::move(event));
  }
  signal_wake(live->wake_state);
}

auto register_runtime_map_events(mln_runtime runtime, mln_map map) -> void {
  auto* live = handle_table<RuntimeObject>().try_resolve(runtime);
  if (live == nullptr || map == MLN_HANDLE_NULL) {
    return;
  }

  const std::scoped_lock lock(live->event_mutex);
  live->event_maps.insert(map);
}

auto discard_runtime_map_events(mln_runtime runtime, mln_map map) -> void {
  auto* live = handle_table<RuntimeObject>().try_resolve(runtime);
  if (live == nullptr || map == MLN_HANDLE_NULL) {
    return;
  }

  const std::scoped_lock lock(live->event_mutex);
  live->event_maps.erase(map);
  std::erase_if(live->events, [map](const auto& event) -> bool {
    return event.source == map;
  });
}

}  // namespace mln::core
