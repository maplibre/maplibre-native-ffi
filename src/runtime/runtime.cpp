#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
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
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/expected.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/run_loop.hpp>

#include "runtime/runtime.hpp"

#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "notification/notification.hpp"
#include "operation/operation.hpp"

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

template <>
struct HandleTraits<EventBatchObject> {
  static constexpr auto kind = HandleKind::EventBatch;
  static constexpr auto leasable = true;
};

}  // namespace mln::core

namespace {
enum : std::uint32_t {
  MLN_OFFLINE_OPERATION_AMBIENT_CACHE = 1,
  MLN_OFFLINE_OPERATION_REGION_CREATE = 2,
  MLN_OFFLINE_OPERATION_REGION_GET = 3,
  MLN_OFFLINE_OPERATION_REGIONS_LIST = 4,
  MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE = 5,
  MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA = 6,
  MLN_OFFLINE_OPERATION_REGION_GET_STATUS = 7,
  MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED = 8,
  MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE = 9,
  MLN_OFFLINE_OPERATION_REGION_INVALIDATE = 10,
  MLN_OFFLINE_OPERATION_REGION_DELETE = 11,
  MLN_OFFLINE_OPERATION_SET_MAXIMUM_AMBIENT_CACHE_SIZE = 12,
};

constexpr auto MLN_RUNTIME_OPERATION_CREATE = UINT32_C(0x10000);
constexpr auto MLN_RUNTIME_OPERATION_BARRIER = UINT32_C(0x10001);
constexpr auto MLN_RUNTIME_OPERATION_CLOSE = UINT32_C(0x10002);

enum : std::uint32_t {
  MLN_OFFLINE_OPERATION_RESULT_NONE = 0,
  MLN_OFFLINE_OPERATION_RESULT_REGION = 1,
  MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION = 2,
  MLN_OFFLINE_OPERATION_RESULT_REGION_LIST = 3,
  MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS = 4,
};

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
MLN_ASSERT_EVENT_MASK_BIT(COMMAND_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(OFFLINE_REGION_STATUS_CHANGED);
MLN_ASSERT_EVENT_MASK_BIT(OFFLINE_REGION_RESPONSE_ERROR);
MLN_ASSERT_EVENT_MASK_BIT(OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED);

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

// Each event keeps a uint32_t message length, so clamp oversized diagnostics
// before they enter the shared queue arena.
auto truncated_event_message(std::string message) -> std::string {
  constexpr auto max_size = static_cast<size_t>(UINT32_MAX) - 1;
  if (message.size() > max_size) {
    message.resize(max_size);
  }
  return message;
}

auto append_runtime_event(
  mln::core::RuntimeEventStorage& storage, mln_runtime_event event,
  std::string message
) -> void {
  message = truncated_event_message(std::move(message));
  event.message_size = static_cast<uint32_t>(message.size());
  event.message_offset =
    message.empty() ? 0 : static_cast<uint64_t>(storage.messages.size());
  storage.events.push_back(event);
  const auto prior_messages_size = storage.messages.size();
  try {
    if (!message.empty()) {
      storage.messages.append(message);
      storage.messages.push_back('\0');
    }
  } catch (...) {
    storage.messages.resize(prior_messages_size);
    storage.events.pop_back();
    throw;
  }
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
) -> mbgl::OfflineRegionDefinition {
  switch (definition.type) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID: {
      const auto& tile = definition.data.tile_pyramid;
      auto bounds = mbgl::LatLngBounds::hull(
        {tile.bounds.southwest.latitude, tile.bounds.southwest.longitude},
        {tile.bounds.northeast.latitude, tile.bounds.northeast.longitude}
      );
      return mbgl::OfflineTilePyramidRegionDefinition{
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
      return mbgl::OfflineGeometryRegionDefinition{
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

auto to_c_download_state(mbgl::OfflineRegionDownloadState state) -> uint32_t {
  switch (state) {
    case mbgl::OfflineRegionDownloadState::Inactive:
      return MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE;
    case mbgl::OfflineRegionDownloadState::Active:
      return MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE;
  }
  assert(false);
  return MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE;
}

auto to_c_status(const mbgl::OfflineRegionStatus& status)
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
  -> std::optional<mbgl::OfflineRegionDownloadState> {
  switch (state) {
    case MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE:
      return mbgl::OfflineRegionDownloadState::Inactive;
    case MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE:
      return mbgl::OfflineRegionDownloadState::Active;
    default:
      return std::nullopt;
  }
}

auto to_c_resource_error_reason(mbgl::Response::Error::Reason reason)
  -> uint32_t {
  switch (reason) {
    case mbgl::Response::Error::Reason::Success:
      return MLN_RESOURCE_ERROR_REASON_NONE;
    case mbgl::Response::Error::Reason::NotFound:
      return MLN_RESOURCE_ERROR_REASON_NOT_FOUND;
    case mbgl::Response::Error::Reason::Server:
      return MLN_RESOURCE_ERROR_REASON_SERVER;
    case mbgl::Response::Error::Reason::Connection:
      return MLN_RESOURCE_ERROR_REASON_CONNECTION;
    case mbgl::Response::Error::Reason::RateLimit:
      return MLN_RESOURCE_ERROR_REASON_RATE_LIMIT;
    case mbgl::Response::Error::Reason::Other:
      return MLN_RESOURCE_ERROR_REASON_OTHER;
  }
  assert(false);
  return MLN_RESOURCE_ERROR_REASON_OTHER;
}

auto to_c_region_data(const mbgl::OfflineRegion& region)
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
    const auto* tile = std::get_if<mbgl::OfflineTilePyramidRegionDefinition>(
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
    const auto* geometry = std::get_if<mbgl::OfflineGeometryRegionDefinition>(
      &region.getDefinition()
    )
  ) {
    data.definition_type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
    data.style_url = geometry->styleURL;
    data.min_zoom = geometry->minZoom;
    data.max_zoom = geometry->maxZoom;
    data.pixel_ratio = geometry->pixelRatio;
    data.include_ideographs = geometry->includeIdeographs;
    data.geometry =
      mln::core::serialize_geojson(mbgl::GeoJSON{geometry->geometry});
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
  const std::scoped_lock event_lock(runtime->event_queue->mutex);
  return runtime->event_queue->observed_offline_regions.contains(region_id);
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

  auto event = mln_runtime_event{
    .type = type,
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .source = runtime->self,
    .code = 0,
    .payload_type = payload_type,
    .message_offset = 0,
    .message_size = 0,
    .payload = payload
  };

  {
    const std::scoped_lock event_lock(runtime->event_queue->mutex);
    if (!runtime->event_queue->observed_offline_regions.contains(region_id)) {
      return;
    }
    append_runtime_event(
      runtime->event_queue->pending, event, std::move(message)
    );
  }
  runtime->event_queue->notification_endpoint->mark_ready();
}

class OfflineRegionRuntimeObserver final : public mbgl::OfflineRegionObserver {
 public:
  OfflineRegionRuntimeObserver(
    std::shared_ptr<mln::core::OfflineRegionEventState> state,
    mln_offline_region_id region_id
  )
      : state_(std::move(state)), region_id_(region_id) {}

  void statusChanged(mbgl::OfflineRegionStatus status) override {
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

  void responseError(mbgl::Response::Error error) override {
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
  const std::scoped_lock lock(runtime->event_queue->mutex);
  if (observed) {
    runtime->event_queue->observed_offline_regions.insert(region_id);
  } else {
    runtime->event_queue->observed_offline_regions.erase(region_id);
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

auto to_c_region_data_list(const mbgl::OfflineRegions& native_regions)
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

template <typename Schedule>
auto schedule_registered_offline_operation(
  mln::core::RuntimeObject* runtime, uint32_t kind,
  mln_operation* out_operation_id, Schedule&& schedule
) -> mln_status {
  auto state = std::shared_ptr<mln::core::OperationObject>{};
  const auto status = mln::core::register_operation(
    runtime->event_queue->notification_source, kind, false, {},
    out_operation_id, state
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  static_cast<void>(
    mln::core::associate_runtime_operation_with_current_submission(
      runtime, state
    )
  );
  const auto operation_id = *out_operation_id;
  try {
    std::forward<Schedule>(schedule)(state);
  } catch (...) {
    state->complete(
      MLN_STATUS_NATIVE_ERROR,
      exception_message(
        std::current_exception(), "offline operation submission failed"
      ),
      std::any{std::monostate{}}
    );
    mln::core::abandon_operation(operation_id);
    *out_operation_id = MLN_HANDLE_NULL;
    throw;
  }
  return MLN_STATUS_OK;
}

template <typename Result>
auto complete_offline_operation(
  const std::shared_ptr<mln::core::OperationObject>& state,
  int32_t result_status, Result result, std::string message = {}
) -> void {
  state->complete(
    static_cast<mln_status>(result_status), std::move(message),
    std::any{std::move(result)}
  );
}

auto complete_offline_operation_error(
  const std::shared_ptr<mln::core::OperationObject>& state, int32_t status,
  std::string message
) -> void {
  complete_offline_operation(
    state, status, std::monostate{}, std::move(message)
  );
}

auto complete_from_exception(
  const std::shared_ptr<mln::core::OperationObject>& state,
  std::exception_ptr exception, const char* fallback
) -> void {
  if (exception) {
    complete_offline_operation_error(
      state, MLN_STATUS_NATIVE_ERROR, exception_message(exception, fallback)
    );
    return;
  }
  complete_offline_operation(state, MLN_STATUS_OK, std::monostate{});
}

auto validate_runtime_options(const mln_runtime_options* options)
  -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("runtime options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
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
  if (options->notification_source == MLN_HANDLE_NULL) {
    mln::core::set_thread_error(
      "mln_runtime_options.notification_source must be live"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

}  // namespace

namespace mln::core {

namespace {

auto database_source_for_runtime(RuntimeObject* runtime)
  -> std::shared_ptr<mbgl::DatabaseFileSource> {
  if (runtime->database_source != nullptr) {
    return runtime->database_source;
  }

  auto source = mbgl::FileSourceManager::get()->getFileSource(
    mbgl::FileSourceType::Database, resource_options_for_runtime(runtime->self),
    mbgl::ClientOptions()
  );
  // MapLibre is built without RTTI. The registered FileSourceType::Database
  // factory always returns a DatabaseFileSource.
  auto database = std::static_pointer_cast<mbgl::DatabaseFileSource>(source);
  runtime->database_source = database;
  return runtime->database_source;
}

}  // namespace

auto validate_runtime(mln_runtime runtime, RuntimeObject*& out_runtime)
  -> mln_status {
  out_runtime = handle_table<RuntimeObject>().resolve(runtime);
  return out_runtime == nullptr ? MLN_STATUS_INVALID_ARGUMENT : MLN_STATUS_OK;
}

auto lease_runtime(mln_runtime runtime) -> std::shared_ptr<RuntimeObject> {
  return handle_table<RuntimeObject>().lease(runtime);
}

namespace {

struct CurrentRuntimeSubmission {
  RuntimeObject* runtime;
  std::shared_ptr<RuntimeObject> retained_runtime;
  uint64_t sequence;
  bool claimed_by_operation = false;
};

auto current_runtime_submission() noexcept -> CurrentRuntimeSubmission*& {
  static thread_local auto* submission =
    static_cast<CurrentRuntimeSubmission*>(nullptr);
  return submission;
}

auto erase_tracked_submission(
  const std::shared_ptr<RuntimeObject>& runtime, uint64_t sequence
) noexcept -> void {
  {
    const std::scoped_lock lock(runtime->terminal_mutex);
    runtime->pending_submissions.erase(sequence);
  }
  runtime->terminal_condition.notify_all();
}

auto erase_runtime_barrier(
  const std::shared_ptr<RuntimeObject>& runtime, uint64_t sequence
) noexcept -> void {
  {
    const std::scoped_lock lock(runtime->terminal_mutex);
    runtime->pending_barriers.erase(sequence);
    runtime->pending_submissions.erase(sequence);
  }
  runtime->terminal_condition.notify_all();
}

auto complete_ready_runtime_barriers(
  const std::shared_ptr<RuntimeObject>& runtime
) noexcept -> void {
  while (true) {
    auto barrier = std::shared_ptr<OperationObject>{};
    {
      const std::scoped_lock lock(runtime->terminal_mutex);
      const auto pending = runtime->pending_barriers.begin();
      if (
        pending == runtime->pending_barriers.end() ||
        (!runtime->pending_submissions.empty() &&
         *runtime->pending_submissions.begin() < pending->first)
      ) {
        return;
      }
      barrier = std::move(pending->second);
      runtime->pending_barriers.erase(pending);
    }
    barrier->complete(MLN_STATUS_OK, {}, std::any{std::monostate{}});
  }
}

auto finish_tracked_submission(
  const std::shared_ptr<RuntimeObject>& runtime, uint64_t sequence
) noexcept -> void {
  erase_tracked_submission(runtime, sequence);
  complete_ready_runtime_barriers(runtime);
}

}  // namespace

auto dispatch_runtime_sync(
  mln_runtime runtime, std::function<mln_status()> function
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock commit_lock(live->submission_mutex);
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto lease = ControlLease{&live->control};
  const auto sequence = live->next_submission_sequence++;
  {
    const std::scoped_lock terminal_lock(live->terminal_mutex);
    live->pending_submissions.insert(sequence);
  }
  auto context = CurrentRuntimeSubmission{
    .runtime = live.get(),
    .retained_runtime = live,
    .sequence = sequence,
  };
  try {
    const auto status = live->executor.invoke_sync(
      [lease = std::move(lease), &context,
       function = std::move(function)]() mutable -> mln_status {
        static_cast<void>(lease);
        auto* previous = current_runtime_submission();
        current_runtime_submission() = &context;
        try {
          const auto result = std::invoke(std::move(function));
          current_runtime_submission() = previous;
          return result;
        } catch (...) {
          current_runtime_submission() = previous;
          throw;
        }
      }
    );
    if (!context.claimed_by_operation) {
      finish_tracked_submission(live, sequence);
    }
    return status;
  } catch (...) {
    if (!context.claimed_by_operation) {
      finish_tracked_submission(live, sequence);
    }
    throw;
  }
}

auto associate_runtime_operation_with_current_submission(
  RuntimeObject* runtime, const std::shared_ptr<OperationObject>& operation
) noexcept -> bool {
  auto* context = current_runtime_submission();
  if (
    context == nullptr || context->runtime != runtime ||
    context->claimed_by_operation || operation == nullptr
  ) {
    return false;
  }
  context->claimed_by_operation = true;
  operation->set_terminal_callback(
    [retained = context->retained_runtime,
     sequence = context->sequence]() noexcept -> void {
      finish_tracked_submission(retained, sequence);
    }
  );
  return true;
}

namespace {

auto mark_runtime_submission_terminal(
  const std::shared_ptr<RuntimeObject>& runtime, uint64_t sequence
) noexcept -> void {
  finish_tracked_submission(runtime, sequence);
}

}  // namespace

auto submit_runtime_command(
  const std::shared_ptr<RuntimeObject>& runtime,
  std::function<void(uint64_t)> function, uint64_t* out_command_id,
  std::atomic<uint64_t>* latest_command_id
) -> mln_status {
  if (runtime == nullptr || out_command_id == nullptr) {
    set_thread_error("runtime and out_command_id must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock commit_lock(runtime->submission_mutex);
  if (!runtime->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto lease = ControlLease{&runtime->control};
  const auto command_id = runtime->next_command_id++;
  const auto sequence = runtime->next_submission_sequence++;
  if (latest_command_id != nullptr) {
    latest_command_id->store(command_id);
  }
  {
    const std::scoped_lock terminal_lock(runtime->terminal_mutex);
    runtime->pending_submissions.insert(sequence);
  }
  try {
    runtime->executor.invoke(
      [runtime, lease = std::move(lease), command_id, sequence,
       function = std::move(function)]() mutable -> void {
        static_cast<void>(lease);
        try {
          std::invoke(std::move(function), command_id);
        } catch (...) {
          // The command implementation reports its own terminal failure event.
        }
        mark_runtime_submission_terminal(runtime, sequence);
      }
    );
  } catch (...) {
    mark_runtime_submission_terminal(runtime, sequence);
    set_thread_error("runtime command submission failed");
    return MLN_STATUS_NATIVE_ERROR;
  }
  *out_command_id = command_id;
  return MLN_STATUS_OK;
}

auto submit_runtime_operation(
  const std::shared_ptr<RuntimeObject>& runtime,
  const std::shared_ptr<OperationObject>& operation,
  std::function<void()> function
) -> mln_status {
  if (runtime == nullptr || operation == nullptr) {
    set_thread_error("runtime and operation must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock commit_lock(runtime->submission_mutex);
  if (!runtime->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto lease = ControlLease{&runtime->control};
  const auto sequence = runtime->next_submission_sequence++;
  {
    const std::scoped_lock terminal_lock(runtime->terminal_mutex);
    runtime->pending_submissions.insert(sequence);
  }
  operation->set_terminal_callback([runtime, sequence]() noexcept -> void {
    mark_runtime_submission_terminal(runtime, sequence);
  });
  try {
    runtime->executor.invoke(
      [operation, lease = std::move(lease),
       function = std::move(function)]() mutable -> void {
        static_cast<void>(lease);
        try {
          std::invoke(std::move(function));
        } catch (...) {
          operation->complete(
            MLN_STATUS_NATIVE_ERROR, "runtime operation submission failed",
            std::any{std::monostate{}}
          );
        }
      }
    );
  } catch (...) {
    operation->complete(
      MLN_STATUS_NATIVE_ERROR, "runtime operation submission failed",
      std::any{std::monostate{}}
    );
    set_thread_error("runtime operation submission failed");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

namespace {

struct RuntimeCreationResult {
  RuntimeCreationResult() = default;
  RuntimeCreationResult(const RuntimeCreationResult&) = delete;
  RuntimeCreationResult(RuntimeCreationResult&&) = delete;
  auto operator=(const RuntimeCreationResult&)
    -> RuntimeCreationResult& = delete;
  auto operator=(RuntimeCreationResult&&) -> RuntimeCreationResult& = delete;

  ~RuntimeCreationResult() {
    if (!published && runtime != nullptr) {
      runtime->executor.stop();
      if (runtime->platform_context != nullptr) {
        unregister_platform_context(runtime->platform_context);
      }
      runtime.reset();
    }
  }

  std::shared_ptr<RuntimeObject> runtime;
  bool published = false;
};

}  // namespace

auto create_runtime_start(
  const mln_runtime_options* options, mln_operation* out_operation
) -> mln_status {
  const auto options_status = validate_runtime_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  const auto notification_source =
    notification_source_from_handle(options->notification_source);
  if (notification_source == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto result = std::make_shared<RuntimeCreationResult>();
  result->runtime = std::make_shared<RuntimeObject>();
  auto asset_path = options->asset_path == nullptr
                      ? std::string{}
                      : std::string{options->asset_path};
  auto cache_path = options->cache_path == nullptr
                      ? std::string{}
                      : std::string{options->cache_path};
  const auto event_mask = options->event_mask;

  auto state = std::shared_ptr<OperationObject>{};
  const auto register_status = register_operation(
    notification_source, MLN_RUNTIME_OPERATION_CREATE, false, {}, out_operation,
    state
  );
  if (register_status != MLN_STATUS_OK) {
    return register_status;
  }

  auto creation_work =
    [state, result, notification_source, asset_path = std::move(asset_path),
     cache_path = std::move(cache_path), event_mask]() mutable -> void {
    try {
      auto runtime = result->runtime;
      runtime->executor.start(
        [runtime, notification_source, asset_path = std::move(asset_path),
         cache_path = std::move(cache_path), event_mask]() mutable -> void {
          runtime->event_state = std::make_shared<RuntimeEventState>();
          runtime->event_state->mask.store(
            event_mask, std::memory_order_relaxed
          );
          runtime->event_queue = std::make_shared<RuntimeEventQueueState>();
          runtime->event_queue->notification_source = notification_source;
          runtime->asset_path = std::move(asset_path);
          runtime->cache_path = std::move(cache_path);
          runtime->offline_event_state =
            std::make_shared<OfflineRegionEventState>();
          runtime->offline_event_state->runtime = runtime.get();
          runtime->offline_event_state->alive = true;
          runtime->resource_transform_state =
            std::make_shared<ResourceTransformState>();
          runtime->http_header_transform_state =
            std::make_shared<HttpHeaderTransformState>();
          runtime->resource_provider_state =
            std::make_shared<ResourceProviderState>();
          runtime->platform_context = reserve_platform_context();
        }
      );
      state->complete(MLN_STATUS_OK, {}, std::any{std::move(result)});
    } catch (...) {
      state->complete(
        MLN_STATUS_NATIVE_ERROR,
        exception_message(std::current_exception(), "runtime creation failed"),
        std::any{std::monostate{}}
      );
    }
  };
  try {
    std::thread(creation_work).detach();
  } catch (...) {
    creation_work();
  }
  return MLN_STATUS_OK;
}

auto create_runtime_take_result(
  mln_operation operation, mln_runtime* out_runtime
) -> mln_status {
  if (out_runtime == nullptr || *out_runtime != MLN_HANDLE_NULL) {
    set_thread_error("out_runtime must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<std::shared_ptr<RuntimeCreationResult>>(
    operation, MLN_RUNTIME_OPERATION_CREATE,
    [out_runtime](std::shared_ptr<RuntimeCreationResult>& result)
      -> mln_status {
      auto runtime = result->runtime;
      const auto handle = handle_table<RuntimeObject>().insert(runtime);
      runtime->self = handle;
      try {
        runtime->event_queue->notification_endpoint =
          runtime->event_queue->notification_source->associate(
            handle, MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS, true
          );
        if (runtime->event_queue->notification_endpoint == nullptr) {
          static_cast<void>(handle_table<RuntimeObject>().remove(handle));
          runtime->self = MLN_HANDLE_NULL;
          return MLN_STATUS_INVALID_STATE;
        }
      } catch (...) {
        static_cast<void>(handle_table<RuntimeObject>().remove(handle));
        runtime->self = MLN_HANDLE_NULL;
        throw;
      }
      bind_platform_context(runtime->platform_context, handle);
      result->published = true;
      *out_runtime = handle;
      return MLN_STATUS_OK;
    }
  );
}

namespace {

template <typename Mutation>
auto execute_resource_configuration_command(
  const std::shared_ptr<mln::core::RuntimeObject>& runtime,
  std::uint64_t command_id, Mutation&& mutation
) noexcept -> void {
  try {
    std::invoke(std::forward<Mutation>(mutation));
    push_runtime_command_finished(
      runtime->self, MLN_RUNTIME_EVENT_SOURCE_RUNTIME, runtime->self,
      command_id, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, 0
    );
  } catch (const std::exception& exception) {
    push_runtime_command_finished(
      runtime->self, MLN_RUNTIME_EVENT_SOURCE_RUNTIME, runtime->self,
      command_id, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_NATIVE_ERROR, 0,
      exception.what()
    );
  } catch (...) {
    push_runtime_command_finished(
      runtime->self, MLN_RUNTIME_EVENT_SOURCE_RUNTIME, runtime->self,
      command_id, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_NATIVE_ERROR, 0,
      "resource configuration command failed"
    );
  }
}

auto validate_command_output(const std::uint64_t* out_command_id)
  -> mln_status {
  if (out_command_id == nullptr || *out_command_id != 0) {
    set_thread_error("out_command_id must point to zero");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace

auto set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform,
  std::uint64_t* out_command_id
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_command_output(out_command_id) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
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

  const auto context = std::make_shared<RuntimeCallbackContext>(
    transform->user_data, transform->release_user_data
  );
  const auto registration = std::make_shared<ResourceTransformRegistration>(
    ResourceTransformRegistration{
      .callback = transform->callback,
      .context = context,
    }
  );
  const auto state = live->resource_transform_state;
  const auto status = submit_runtime_command(
    live,
    [live, state, registration](std::uint64_t command_id) -> void {
      execute_resource_configuration_command(
        live, command_id, [state, registration]() -> void {
          auto previous = std::shared_ptr<ResourceTransformRegistration>{};
          {
            const std::unique_lock lock(state->mutex);
            previous = std::exchange(state->registration, registration);
          }
        }
      );
    },
    out_command_id
  );
  if (status == MLN_STATUS_OK) context->transfer_to_runtime();
  return status;
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

auto clear_resource_transform(
  mln_runtime runtime, std::uint64_t* out_command_id
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_command_output(out_command_id) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto state = live->resource_transform_state;
  return submit_runtime_command(
    live,
    [live, state](std::uint64_t command_id) -> void {
      execute_resource_configuration_command(
        live, command_id, [state]() -> void {
          auto previous = std::shared_ptr<ResourceTransformRegistration>{};
          {
            const std::unique_lock lock(state->mutex);
            previous = std::exchange(state->registration, nullptr);
          }
        }
      );
    },
    out_command_id
  );
}

auto set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  std::uint64_t* out_command_id
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_command_output(out_command_id) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
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
  set_thread_error(
    "HTTP header transforms are unsupported in the browser because "
    "emscripten_fetch follows redirects itself and cannot drop transformed "
    "headers when the origin changes; serve the request with "
    "mln_runtime_set_resource_provider() instead"
  );
  return MLN_STATUS_UNSUPPORTED;
#endif

  const auto context = std::make_shared<RuntimeCallbackContext>(
    transform->user_data, transform->release_user_data
  );
  const auto registration = std::make_shared<HttpHeaderTransformRegistration>(
    HttpHeaderTransformRegistration{
      .callback = transform->callback,
      .context = context,
    }
  );
  const auto state = live->http_header_transform_state;
  const auto status = submit_runtime_command(
    live,
    [live, state, registration](std::uint64_t command_id) -> void {
      execute_resource_configuration_command(
        live, command_id, [state, registration]() -> void {
          auto previous = std::shared_ptr<HttpHeaderTransformRegistration>{};
          {
            const std::unique_lock lock(state->mutex);
            previous = std::exchange(state->registration, registration);
          }
        }
      );
    },
    out_command_id
  );
  if (status == MLN_STATUS_OK) context->transfer_to_runtime();
  return status;
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

auto clear_http_header_transform(
  mln_runtime runtime, std::uint64_t* out_command_id
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_command_output(out_command_id) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto state = live->http_header_transform_state;
  return submit_runtime_command(
    live,
    [live, state](std::uint64_t command_id) -> void {
      execute_resource_configuration_command(
        live, command_id, [state]() -> void {
          auto previous = std::shared_ptr<HttpHeaderTransformRegistration>{};
          {
            const std::unique_lock lock(state->mutex);
            previous = std::exchange(state->registration, nullptr);
          }
        }
      );
    },
    out_command_id
  );
}

auto run_ambient_cache_operation_start(
  mln_runtime runtime, uint32_t operation, mln_operation* out_operation_id
) -> mln_status {
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
    live, MLN_OFFLINE_OPERATION_AMBIENT_CACHE, out_operation_id,
    [&](auto state) -> void {
      auto callback = [state](std::exception_ptr exception) -> void {
        complete_from_exception(
          state, exception, "ambient cache operation failed"
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
  mln_runtime runtime, std::uint64_t size, mln_operation* out_operation_id
) -> mln_status {
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
    out_operation_id, [&](auto state) -> void {
      database->setMaximumAmbientCacheSize(
        size, [state](std::exception_ptr exception) -> void {
          complete_from_exception(
            state, exception, "setting the maximum ambient cache size failed"
          );
        }
      );
    }
  );
}

auto offline_region_create_start(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size, mln_operation* out_operation_id
) -> mln_status {
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
  auto native_metadata = mbgl::OfflineRegionMetadata{};
  if (metadata_size != 0) {
    native_metadata.resize(metadata_size);
    std::memcpy(native_metadata.data(), metadata, metadata_size);
  }
  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_CREATE, out_operation_id,
    [&](auto state) -> void {
      database->createOfflineRegion(
        native_definition, native_metadata,
        [state](mbgl::expected<mbgl::OfflineRegion, std::exception_ptr> result)
          -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(
                result.error(), "offline region creation failed"
              )
            );
            return;
          }
          auto data = to_c_region_data(result.value());
          if (!data) {
            complete_offline_operation_error(
              state, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(state, MLN_STATUS_OK, std::move(*data));
        }
      );
    }
  );
}

auto offline_region_get_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_operation* out_operation_id
) -> mln_status {
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
    live, MLN_OFFLINE_OPERATION_REGION_GET, out_operation_id,
    [&](auto state) -> void {
      database->getOfflineRegion(
        region_id,
        [state](
          mbgl::expected<std::optional<mbgl::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation(
              state, MLN_STATUS_OK, std::optional<OfflineRegionData>{}
            );
            return;
          }
          auto data = to_c_region_data(region.value());
          if (!data) {
            complete_offline_operation_error(
              state, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(state, MLN_STATUS_OK, std::move(data));
        }
      );
    }
  );
}

auto offline_regions_list_start(
  mln_runtime runtime, mln_operation* out_operation_id
) -> mln_status {
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
    live, MLN_OFFLINE_OPERATION_REGIONS_LIST, out_operation_id,
    [&](auto state) -> void {
      database->listOfflineRegions(
        [state](
          mbgl::expected<mbgl::OfflineRegions, std::exception_ptr> result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region list failed")
            );
            return;
          }
          auto regions = to_c_region_data_list(result.value());
          if (!regions) {
            complete_offline_operation_error(
              state, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(state, MLN_STATUS_OK, std::move(*regions));
        }
      );
    }
  );
}

auto offline_regions_merge_database_start(
  mln_runtime runtime, const char* side_database_path,
  mln_operation* out_operation_id
) -> mln_status {
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

  auto database = database_source_for_runtime(live);
  if (database == nullptr) {
    set_thread_error("database file source is unavailable");
    return MLN_STATUS_NATIVE_ERROR;
  }

  const auto path = std::string{side_database_path};
  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE, out_operation_id,
    [&](auto state) -> void {
      database->mergeOfflineRegions(
        path,
        [state](mbgl::expected<mbgl::OfflineRegions, std::exception_ptr> result)
          -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(
                result.error(), "offline region database merge failed"
              )
            );
            return;
          }
          auto regions = to_c_region_data_list(result.value());
          if (!regions) {
            complete_offline_operation_error(
              state, MLN_STATUS_UNSUPPORTED,
              "offline region definition type is unsupported"
            );
            return;
          }
          complete_offline_operation(state, MLN_STATUS_OK, std::move(*regions));
        }
      );
    }
  );
}

auto offline_region_update_metadata_start(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, mln_operation* out_operation_id
) -> mln_status {
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

  auto native_metadata = mbgl::OfflineRegionMetadata{};
  if (metadata_size != 0) {
    native_metadata.resize(metadata_size);
    std::memcpy(native_metadata.data(), metadata, metadata_size);
  }
  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA, out_operation_id,
    [&](auto state) -> void {
      database->updateOfflineMetadata(
        region_id, native_metadata,
        [database, state, region_id](
          mbgl::expected<mbgl::OfflineRegionMetadata, std::exception_ptr> result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(
                result.error(), "offline region metadata update failed"
              )
            );
            return;
          }
          database->getOfflineRegion(
            region_id,
            [state](
              mbgl::expected<
                std::optional<mbgl::OfflineRegion>, std::exception_ptr>
                result
            ) -> void {
              if (!result) {
                complete_offline_operation_error(
                  state, MLN_STATUS_NATIVE_ERROR,
                  exception_message(result.error(), "offline region get failed")
                );
                return;
              }
              const auto& region = result.value();
              if (!region.has_value()) {
                complete_offline_operation_error(
                  state, MLN_STATUS_INVALID_ARGUMENT, "offline region not found"
                );
                return;
              }
              auto data = to_c_region_data(region.value());
              if (!data) {
                complete_offline_operation_error(
                  state, MLN_STATUS_UNSUPPORTED,
                  "offline region definition type is unsupported"
                );
                return;
              }
              complete_offline_operation(
                state, MLN_STATUS_OK, std::move(*data)
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
  mln_operation* out_operation_id
) -> mln_status {
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
    live, MLN_OFFLINE_OPERATION_REGION_GET_STATUS, out_operation_id,
    [&](auto state) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state](
          mbgl::expected<std::optional<mbgl::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, MLN_STATUS_INVALID_ARGUMENT, "offline region not found"
            );
            return;
          }
          database->getOfflineRegionStatus(
            region.value(),
            [state](
              mbgl::expected<mbgl::OfflineRegionStatus, std::exception_ptr>
                result
            ) -> void {
              if (!result) {
                complete_offline_operation_error(
                  state, MLN_STATUS_NATIVE_ERROR,
                  exception_message(
                    result.error(), "offline region status query failed"
                  )
                );
                return;
              }
              complete_offline_operation(
                state, MLN_STATUS_OK, to_c_status(result.value())
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
  mln_operation* out_operation_id
) -> mln_status {
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
  auto offline_event_state = live->offline_event_state;

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED, out_operation_id,
    [&, offline_event_state](auto state) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, region_id, observed, offline_event_state](
          mbgl::expected<std::optional<mbgl::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, MLN_STATUS_INVALID_ARGUMENT, "offline region not found"
            );
            return;
          }
          {
            const std::scoped_lock event_state_lock(offline_event_state->mutex);
            if (
              !offline_event_state->alive ||
              offline_event_state->runtime == nullptr
            ) {
              complete_offline_operation(
                state, MLN_STATUS_OK, std::monostate{}
              );
              return;
            }
            set_offline_region_observed_flag(
              offline_event_state->runtime, region_id, observed
            );
          }
          auto observer = observed
                            ? std::make_unique<OfflineRegionRuntimeObserver>(
                                offline_event_state, region_id
                              )
                            : nullptr;
          database->setOfflineRegionObserver(
            region.value(), std::move(observer)
          );
          complete_offline_operation(state, MLN_STATUS_OK, std::monostate{});
        }
      );
    }
  );
}

auto offline_region_set_download_state_start(
  mln_runtime runtime, OfflineRegionDownloadStateRequest request,
  mln_operation* out_operation_id
) -> mln_status {
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
    live, MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE, out_operation_id,
    [&](auto state) -> void {
      database->getOfflineRegion(
        request.region_id,
        [database, state, native_state](
          mbgl::expected<std::optional<mbgl::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, MLN_STATUS_INVALID_ARGUMENT, "offline region not found"
            );
            return;
          }
          database->setOfflineRegionDownloadState(
            region.value(), *native_state
          );
          complete_offline_operation(state, MLN_STATUS_OK, std::monostate{});
        }
      );
    }
  );
}

auto offline_region_invalidate_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_operation* out_operation_id
) -> mln_status {
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
    live, MLN_OFFLINE_OPERATION_REGION_INVALIDATE, out_operation_id,
    [&](auto state) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state](
          mbgl::expected<std::optional<mbgl::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, MLN_STATUS_INVALID_ARGUMENT, "offline region not found"
            );
            return;
          }
          database->invalidateOfflineRegion(
            region.value(), [state](std::exception_ptr exception) -> void {
              complete_from_exception(
                state, exception, "offline region invalidation failed"
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
  mln_operation* out_operation_id
) -> mln_status {
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
  auto offline_event_state = live->offline_event_state;

  return schedule_registered_offline_operation(
    live, MLN_OFFLINE_OPERATION_REGION_DELETE, out_operation_id,
    [&, offline_event_state](auto state) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, region_id, offline_event_state](
          mbgl::expected<std::optional<mbgl::OfflineRegion>, std::exception_ptr>
            result
        ) -> void {
          if (!result) {
            complete_offline_operation_error(
              state, MLN_STATUS_NATIVE_ERROR,
              exception_message(result.error(), "offline region get failed")
            );
            return;
          }
          const auto& region = result.value();
          if (!region.has_value()) {
            complete_offline_operation_error(
              state, MLN_STATUS_INVALID_ARGUMENT, "offline region not found"
            );
            return;
          }
          {
            const std::scoped_lock event_state_lock(offline_event_state->mutex);
            if (
              offline_event_state->alive &&
              offline_event_state->runtime != nullptr
            ) {
              set_offline_region_observed_flag(
                offline_event_state->runtime, region_id, false
              );
            }
          }
          database->setOfflineRegionObserver(region.value(), nullptr);
          database->setOfflineRegionDownloadState(
            region.value(), mbgl::OfflineRegionDownloadState::Inactive
          );
          database->deleteOfflineRegion(
            region.value(), [state](std::exception_ptr exception) -> void {
              complete_from_exception(
                state, exception, "offline region deletion failed"
              );
            }
          );
        }
      );
    }
  );
}

auto offline_region_create_take_result(
  mln_operation operation, mln_offline_region_snapshot* out_region
) -> mln_status {
  if (out_region == nullptr || *out_region != MLN_HANDLE_NULL) {
    set_thread_error("out_region must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<OfflineRegionData>(
    operation, MLN_OFFLINE_OPERATION_REGION_CREATE,
    [out_region](OfflineRegionData& result) -> mln_status {
      *out_region = register_offline_region_snapshot_from_result(
        [&result](OfflineRegionData& destination) -> void {
          destination = std::move(result);
        }
      );
      return MLN_STATUS_OK;
    }
  );
}

auto offline_region_get_take_result(
  mln_operation operation, mln_offline_region_snapshot* out_region,
  bool* out_found
) -> mln_status {
  if (
    out_region == nullptr || *out_region != MLN_HANDLE_NULL ||
    out_found == nullptr
  ) {
    set_thread_error(
      "out_region must point to the null handle and out_found must not be null"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<std::optional<OfflineRegionData>>(
    operation, MLN_OFFLINE_OPERATION_REGION_GET,
    [out_region,
     out_found](std::optional<OfflineRegionData>& result) -> mln_status {
      if (!result.has_value()) {
        *out_found = false;
        return MLN_STATUS_OK;
      }
      *out_region = register_offline_region_snapshot_from_result(
        [&result](OfflineRegionData& destination) -> void {
          destination = std::move(*result);
        }
      );
      *out_found = true;
      return MLN_STATUS_OK;
    }
  );
}

auto offline_regions_list_take_result(
  mln_operation operation, mln_offline_region_list* out_regions
) -> mln_status {
  if (out_regions == nullptr || *out_regions != MLN_HANDLE_NULL) {
    set_thread_error("out_regions must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<std::vector<OfflineRegionData>>(
    operation, MLN_OFFLINE_OPERATION_REGIONS_LIST,
    [out_regions](std::vector<OfflineRegionData>& result) -> mln_status {
      *out_regions = register_offline_region_list_from_result(
        [&result](std::vector<OfflineRegionData>& destination) -> void {
          destination = std::move(result);
        }
      );
      return MLN_STATUS_OK;
    }
  );
}

auto offline_regions_merge_database_take_result(
  mln_operation operation, mln_offline_region_list* out_regions
) -> mln_status {
  if (out_regions == nullptr || *out_regions != MLN_HANDLE_NULL) {
    set_thread_error("out_regions must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<std::vector<OfflineRegionData>>(
    operation, MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE,
    [out_regions](std::vector<OfflineRegionData>& result) -> mln_status {
      *out_regions = register_offline_region_list_from_result(
        [&result](std::vector<OfflineRegionData>& destination) -> void {
          destination = std::move(result);
        }
      );
      return MLN_STATUS_OK;
    }
  );
}

auto offline_region_update_metadata_take_result(
  mln_operation operation, mln_offline_region_snapshot* out_region
) -> mln_status {
  if (out_region == nullptr || *out_region != MLN_HANDLE_NULL) {
    set_thread_error("out_region must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<OfflineRegionData>(
    operation, MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA,
    [out_region](OfflineRegionData& result) -> mln_status {
      *out_region = register_offline_region_snapshot_from_result(
        [&result](OfflineRegionData& destination) -> void {
          destination = std::move(result);
        }
      );
      return MLN_STATUS_OK;
    }
  );
}

auto offline_region_get_status_take_result(
  mln_operation operation, mln_offline_region_status* out_status
) -> mln_status {
  if (
    out_status == nullptr ||
    out_status->size < sizeof(mln_offline_region_status)
  ) {
    set_thread_error("out_status must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return take_operation_result<mln_offline_region_status>(
    operation, MLN_OFFLINE_OPERATION_REGION_GET_STATUS,
    [out_status](mln_offline_region_status& result) -> mln_status {
      *out_status = result;
      return MLN_STATUS_OK;
    }
  );
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
  mln_runtime runtime, const mln_resource_provider* provider,
  std::uint64_t* out_command_id
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_command_output(out_command_id) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
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

  const auto context = std::make_shared<RuntimeCallbackContext>(
    provider->user_data, provider->release_user_data
  );
  const auto copied = ResourceProvider{
    .callback = provider->callback,
    .context = context,
  };
  const auto state = live->resource_provider_state;
  const auto status = submit_runtime_command(
    live,
    [live, state, copied](std::uint64_t command_id) -> void {
      execute_resource_configuration_command(
        live, command_id, [state, copied]() -> void {
          auto previous = ResourceProvider{};
          {
            const std::unique_lock lock(state->mutex);
            previous = std::exchange(state->provider, copied);
          }
        }
      );
    },
    out_command_id
  );
  if (status == MLN_STATUS_OK) context->transfer_to_runtime();
  return status;
}

auto clear_resource_provider(mln_runtime runtime, std::uint64_t* out_command_id)
  -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_command_output(out_command_id) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto state = live->resource_provider_state;
  return submit_runtime_command(
    live,
    [live, state](std::uint64_t command_id) -> void {
      execute_resource_configuration_command(
        live, command_id, [state]() -> void {
          auto previous = ResourceProvider{};
          {
            const std::unique_lock lock(state->mutex);
            previous = std::exchange(state->provider, ResourceProvider{});
          }
        }
      );
    },
    out_command_id
  );
}

namespace {

auto prior_runtime_submissions_terminal(
  const RuntimeObject& runtime, uint64_t sequence
) noexcept -> bool {
  return runtime.pending_submissions.empty() ||
         *runtime.pending_submissions.begin() >= sequence;
}

auto wait_for_prior_runtime_submissions(
  const std::shared_ptr<RuntimeObject>& runtime, uint64_t sequence
) noexcept -> void {
  auto lock = std::unique_lock{runtime->terminal_mutex};
  runtime->terminal_condition.wait(lock, [&]() noexcept -> bool {
    return prior_runtime_submissions_terminal(*runtime, sequence);
  });
}

auto release_runtime_reachable_state(
  const std::shared_ptr<RuntimeObject>& runtime
) -> void {
  {
    auto& transform_state = *runtime->resource_transform_state;
    auto previous = std::shared_ptr<ResourceTransformRegistration>{};
    {
      const std::unique_lock transform_lock(transform_state.mutex);
      previous = std::exchange(transform_state.registration, nullptr);
    }
  }
  {
    auto& transform_state = *runtime->http_header_transform_state;
    auto previous = std::shared_ptr<HttpHeaderTransformRegistration>{};
    {
      const std::unique_lock transform_lock(transform_state.mutex);
      previous = std::exchange(transform_state.registration, nullptr);
    }
  }
  {
    auto& provider_state = *runtime->resource_provider_state;
    auto previous = ResourceProvider{};
    {
      const std::unique_lock provider_lock(provider_state.mutex);
      previous = std::exchange(provider_state.provider, ResourceProvider{});
    }
  }
  {
    const std::scoped_lock state_lock(runtime->offline_event_state->mutex);
    runtime->offline_event_state->alive = false;
    runtime->offline_event_state->runtime = nullptr;
    const std::scoped_lock event_lock(runtime->event_queue->mutex);
    runtime->event_queue->observed_offline_regions.clear();
  }

  auto event_endpoint = std::shared_ptr<NotificationEndpoint>{};
  {
    const std::scoped_lock event_lock(runtime->event_queue->mutex);
    runtime->event_queue->pending.events.clear();
    runtime->event_queue->pending.messages.clear();
    runtime->event_queue->event_maps.clear();
    runtime->event_queue->observed_offline_regions.clear();
    event_endpoint = std::move(runtime->event_queue->notification_endpoint);
  }
  event_endpoint.reset();
  runtime->event_queue->notification_source.reset();
  runtime->database_source.reset();
}

}  // namespace

auto runtime_barrier_start(mln_runtime runtime, mln_operation* out_operation)
  -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = std::shared_ptr<OperationObject>{};
  const auto register_status = register_operation(
    live->event_queue->notification_source, MLN_RUNTIME_OPERATION_BARRIER,
    false, {}, out_operation, state
  );
  if (register_status != MLN_STATUS_OK) {
    return register_status;
  }

  auto sequence = uint64_t{0};
  {
    const std::scoped_lock commit_lock(live->submission_mutex);
    if (!live->control.acquire()) {
      abandon_operation(*out_operation);
      *out_operation = MLN_HANDLE_NULL;
      return MLN_STATUS_INVALID_STATE;
    }
    auto lease = ControlLease{&live->control};
    sequence = live->next_submission_sequence++;
    {
      const std::scoped_lock terminal_lock(live->terminal_mutex);
      live->pending_submissions.insert(sequence);
      live->pending_barriers.emplace(sequence, state);
    }
    state->set_terminal_callback([live, sequence]() noexcept -> void {
      erase_runtime_barrier(live, sequence);
    });
    try {
      live->executor.invoke([lease = std::move(lease)]() mutable -> void {
        static_cast<void>(lease);
      });
    } catch (...) {
      state->complete(
        MLN_STATUS_NATIVE_ERROR,
        exception_message(std::current_exception(), "runtime barrier failed"),
        std::any{std::monostate{}}
      );
      return MLN_STATUS_OK;
    }
  }
  complete_ready_runtime_barriers(live);
  return MLN_STATUS_OK;
}

auto close_runtime_start(mln_runtime runtime, mln_operation* out_operation)
  -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto state = std::shared_ptr<OperationObject>{};
  const auto register_status = register_operation(
    live->event_queue->notification_source, MLN_RUNTIME_OPERATION_CLOSE, false,
    {}, out_operation, state
  );
  if (register_status != MLN_STATUS_OK) {
    return register_status;
  }

  auto sequence = uint64_t{0};
  {
    const std::scoped_lock commit_lock(live->submission_mutex);
    const auto close_status = live->control.begin_close();
    if (close_status != MLN_STATUS_OK) {
      abandon_operation(*out_operation);
      *out_operation = MLN_HANDLE_NULL;
      return close_status;
    }
    sequence = live->next_submission_sequence++;
    {
      const std::scoped_lock terminal_lock(live->terminal_mutex);
      live->pending_submissions.insert(sequence);
    }
    state->set_terminal_callback([live, sequence]() noexcept -> void {
      mark_runtime_submission_terminal(live, sequence);
    });
    static_cast<void>(handle_table<RuntimeObject>().remove(runtime));
  }

  auto close_work = [live, state, sequence]() -> void {
    auto status = MLN_STATUS_OK;
    auto diagnostic = std::string{};
    wait_for_prior_runtime_submissions(live, sequence);
    live->control.wait_for_submissions();
    unregister_platform_context(live->platform_context);
    try {
      live->executor.invoke_sync([live]() -> void {
        release_runtime_reachable_state(live);
      });
    } catch (...) {
      status = MLN_STATUS_NATIVE_ERROR;
      diagnostic =
        exception_message(std::current_exception(), "runtime close failed");
    }
    live->executor.stop();
    state->complete(status, std::move(diagnostic), std::any{std::monostate{}});
  };
  try {
    // Closing stops and joins the runtime executor, so it cannot run on that
    // executor or on the caller that may also service host callbacks.
    std::thread(close_work).detach();
  } catch (...) {
    close_work();
  }
  return MLN_STATUS_OK;
}

auto drain_runtime_events(mln_runtime runtime, mln_event_batch* out_batch)
  -> mln_status {
  if (out_batch == nullptr || *out_batch != MLN_HANDLE_NULL) {
    set_thread_error("out_batch must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto control_lease = ControlLease{&live->control};
  auto queue = live->event_queue;
  auto owned = std::make_shared<EventBatchObject>();
  const auto batch_handle = handle_table<EventBatchObject>().insert(owned);
  {
    const std::scoped_lock lock(queue->mutex);
    owned->storage.events.swap(queue->pending.events);
    owned->storage.messages.swap(queue->pending.messages);
    queue->notification_endpoint->clear_ready();
  }
  *out_batch = batch_handle;
  return MLN_STATUS_OK;
}

auto get_event_batch(
  mln_event_batch batch, mln_runtime_event_batch_view* out_view
) -> mln_status {
  const auto live = handle_table<EventBatchObject>().lease(batch);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    out_view == nullptr || out_view->size < sizeof(mln_runtime_event_batch_view)
  ) {
    set_thread_error("out_view must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_view = mln_runtime_event_batch_view{
    .size = sizeof(mln_runtime_event_batch_view),
    .event_size = sizeof(mln_runtime_event),
    .events =
      live->storage.events.empty() ? nullptr : live->storage.events.data(),
    .event_count = live->storage.events.size(),
    .messages =
      live->storage.messages.empty() ? nullptr : live->storage.messages.data(),
    .messages_size = live->storage.messages.size()
  };
  return MLN_STATUS_OK;
}

auto release_event_batch(mln_event_batch batch) noexcept -> void {
  static_cast<void>(handle_table<EventBatchObject>().remove(batch));
}

auto set_runtime_event_mask(mln_runtime runtime, uint64_t mask) -> mln_status {
  const auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto control_lease = ControlLease{&live->control};
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
  const auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto control_lease = ControlLease{&live->control};
  if (out_mask == nullptr) {
    set_thread_error("out_mask must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_mask = live->event_state->mask.load(std::memory_order_relaxed);
  return MLN_STATUS_OK;
}

auto retain_runtime_map(mln_runtime runtime) -> mln_status {
  const auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.reserve_child()) {
    return MLN_STATUS_INVALID_STATE;
  }
  live->control.commit_child();
  return MLN_STATUS_OK;
}

auto release_runtime_map(mln_runtime runtime) noexcept -> void {
  const auto live = handle_table<RuntimeObject>().try_lease(runtime);
  if (live != nullptr) {
    live->control.release_child();
  }
}

auto runtime_run_loop(RuntimeObject* runtime) -> mbgl::util::RunLoop& {
  return *runtime->executor.run_loop();
}

auto resource_options_for_runtime(mln_runtime runtime)
  -> mbgl::ResourceOptions {
  auto options = mbgl::ResourceOptions::Default();
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
  // committed close clears the registration holds teardown until it returns;
  // one that arrives later finds an empty registration.
  std::shared_lock provider_lock(state->mutex);
  if (state->provider.callback == nullptr) {
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
  return state->registration != nullptr;
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
  const auto registration = state->registration;
  if (registration == nullptr) {
    return MLN_STATUS_OK;
  }

  auto response = mln_resource_transform_response{
    .size = sizeof(mln_resource_transform_response),
    .url = nullptr,
    .context = &out_replacement_url,
  };
  try {
    const auto status = registration->callback(
      registration->context->user_data(), kind, url, &response
    );
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
  const auto registration = state->registration;
  if (registration == nullptr) {
    return {};
  }

  auto headers = HttpHeaders{};
  auto response = mln_http_header_transform_response{
    .size = sizeof(mln_http_header_transform_response),
    .context = &headers,
  };
  try {
    if (
      registration->callback(
        registration->context->user_data(), kind, url, &response
      ) != MLN_STATUS_OK
    ) {
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
  // Observer callbacks may race close; the lease keeps the queue reachable
  // through the copied event and readiness notification.
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return;
  }

  auto event = mln_runtime_event{
    .type = type,
    .source_type = MLN_RUNTIME_EVENT_SOURCE_MAP,
    .source = map,
    .code = code,
    .payload_type = payload_type,
    .message_offset = 0,
    .message_size = 0,
    .payload = payload
  };

  {
    const std::scoped_lock lock(live->event_queue->mutex);
    if (
      map != MLN_HANDLE_NULL && !live->event_queue->event_maps.contains(map)
    ) {
      return;
    }
    // A render draws the latest update, so one unread render-update event
    // covers every invalidation queued behind it. Comparing against the tail
    // alone preserves the order of every other event.
    if (
      type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE &&
      map != MLN_HANDLE_NULL && !live->event_queue->pending.events.empty() &&
      live->event_queue->pending.events.back().type == type &&
      live->event_queue->pending.events.back().source == map
    ) {
      return;
    }
    append_runtime_event(live->event_queue->pending, event, std::move(message));
  }
  live->event_queue->notification_endpoint->mark_ready();
}

auto push_runtime_command_finished(
  mln_runtime runtime, uint32_t source_type, uint64_t source,
  uint64_t command_id, uint32_t disposition, mln_status status,
  uint64_t generation, const char* message
) -> void {
  auto live = lease_runtime(runtime);
  if (live == nullptr) {
    return;
  }
  if (
    source_type != MLN_RUNTIME_EVENT_SOURCE_MAP &&
    !event_selected(live->event_state->mask, MLN_RUNTIME_EVENT_COMMAND_FINISHED)
  ) {
    return;
  }

  auto payload = zeroed_event_payload();
  payload.command_finished = mln_runtime_event_command_finished{
    .command_id = command_id,
    .disposition = disposition,
    .reserved = 0,
    .generation = generation,
  };
  auto event = mln_runtime_event{
    .type = MLN_RUNTIME_EVENT_COMMAND_FINISHED,
    .source_type = source_type,
    .source = source,
    .code = static_cast<int32_t>(status),
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED,
    .message_offset = 0,
    .message_size = 0,
    .payload = payload
  };
  {
    const std::scoped_lock lock(live->event_queue->mutex);
    if (source_type == MLN_RUNTIME_EVENT_SOURCE_MAP) {
      const auto found = live->event_queue->event_maps.find(source);
      if (
        found == live->event_queue->event_maps.end() ||
        !event_selected(found->second->mask, MLN_RUNTIME_EVENT_COMMAND_FINISHED)
      ) {
        return;
      }
    }
    append_runtime_event(
      live->event_queue->pending, event,
      message == nullptr ? std::string{} : std::string{message}
    );
  }
  live->event_queue->notification_endpoint->mark_ready();
}

auto register_runtime_map_events(
  mln_runtime runtime, mln_map map, std::shared_ptr<MapEventState> event_state
) -> void {
  auto live = lease_runtime(runtime);
  if (live == nullptr || map == MLN_HANDLE_NULL || event_state == nullptr) {
    return;
  }

  const std::scoped_lock lock(live->event_queue->mutex);
  live->event_queue->event_maps.insert_or_assign(map, std::move(event_state));
}

auto unregister_runtime_map_events(mln_runtime runtime, mln_map map) -> void {
  auto live = lease_runtime(runtime);
  if (live == nullptr || map == MLN_HANDLE_NULL) {
    return;
  }

  const std::scoped_lock lock(live->event_queue->mutex);
  live->event_queue->event_maps.erase(map);
}

}  // namespace mln::core
