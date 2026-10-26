#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <mln/actor/scheduler.hpp>
#include <mln/storage/database_file_source.hpp>
#include <mln/storage/file_source.hpp>
#include <mln/storage/file_source_manager.hpp>
#include <mln/storage/offline.hpp>
#include <mln/storage/resource_options.hpp>
#include <mln/storage/response.hpp>
#include <mln/storage/sqlite3.hpp>
#include <mln/util/client_options.hpp>
#include <mln/util/expected.hpp>
#include <mln/util/geo.hpp>
#include <mln/util/run_loop.hpp>

#include "runtime/runtime.hpp"

#include "completion/completion.hpp"
#include "completion/completion_result.hpp"
#include "delivery/queued_registration.hpp"
#include "diagnostics/diagnostics.hpp"
#include "diagnostics/string_view.hpp"
#include "execution/process_exit.hpp"
#include "execution/worker_thread.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "resources/custom_resource_provider.hpp"
#include "resources/resource_transform.hpp"
#include "resources/url_rules.hpp"
#include "testing/sync_point.hpp"
#include "wake/wake.hpp"

struct OfflineRegionData {
  mln_offline_region_id id = 0;
  mln_offline_region_definition_type definition_type{};
  std::string style_url;
  mln_lat_lng_bounds bounds{};
  double min_zoom = 0.0;
  double max_zoom = 0.0;
  float pixel_ratio = 0.0F;
  bool include_ideographs = false;
  std::string geometry;
  std::vector<uint8_t> metadata;
};

namespace {
// Each mask constant must be the bit of the event type it selects, written
// against the shift rather than a literal, so an event type added without its
// mask constant fails the build here instead of silently queueing nothing.
#define MLN_ASSERT_EVENT_MASK_BIT(name)                                   \
  static_assert(                                                          \
    MLN_RUNTIME_EVENT_FLAG_##name == 1ULL << MLN_RUNTIME_EVENT_##name,    \
    "MLN_RUNTIME_EVENT_FLAG_" #name " must be the bit for its event type" \
  )

MLN_ASSERT_EVENT_MASK_BIT(MAP_CAMERA_WILL_CHANGE);
MLN_ASSERT_EVENT_MASK_BIT(MAP_CAMERA_IS_CHANGING);
MLN_ASSERT_EVENT_MASK_BIT(MAP_CAMERA_DID_CHANGE);
MLN_ASSERT_EVENT_MASK_BIT(MAP_STYLE_LOADED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_LOADING_STARTED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_LOADING_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_LOADING_FAILED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_IDLE);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_ERROR);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_FRAME_STARTED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_FRAME_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_MAP_STARTED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_RENDER_MAP_FINISHED);
MLN_ASSERT_EVENT_MASK_BIT(MAP_STYLE_IMAGE_MISSING);
MLN_ASSERT_EVENT_MASK_BIT(MAP_TILE_ACTION);
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

auto valid_coordinate(const mln_lat_lng& coordinate) -> bool {
  return std::isfinite(coordinate.latitude) && coordinate.latitude >= -90.0 &&
         coordinate.latitude <= 90.0 && std::isfinite(coordinate.longitude);
}

auto validate_tile_pyramid_definition(
  const mln_offline_tile_pyramid_region_definition& definition
) -> mln_status {
  if (!mln::core::validate_c_string_view(
        definition.style_url, "offline region style_url"
      )) {
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
  if (!mln::core::validate_c_string_view(
        definition.style_url, "offline region style_url"
      )) {
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
        mln::core::string_from_view(tile.style_url),
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
        mln::core::string_from_view(geometry.style_url),
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

auto to_c_download_state(mln::OfflineRegionDownloadState state)
  -> mln_offline_region_download_state {
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

auto to_native_download_state(mln_offline_region_download_state state)
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
  -> mln_resource_error_reason {
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
  if (out_info == nullptr) {
    mln::core::set_thread_error("out_info must not be null");
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
        .style_url =
          {.data = data.style_url.data(), .size = data.style_url.size()},
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
        .style_url =
          {.data = data.style_url.data(), .size = data.style_url.size()},
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
  mln_offline_region_id region_id, mln_runtime_event_type type
) -> bool {
  const std::scoped_lock state_lock(state->mutex);
  auto* runtime = state->runtime;
  if (!state->alive || runtime == nullptr) {
    return false;
  }
  if (!mln::core::event_selected(runtime->event_state->mask, type)) {
    return false;
  }
  return state->observed_regions.contains(region_id);
}

auto push_offline_region_event(
  const std::shared_ptr<mln::core::OfflineRegionEventState>& state,
  mln_offline_region_id region_id, mln_runtime_event_type type,
  mln_runtime_event_payload_type payload_type,
  const mln_runtime_event_payload& payload, std::string message = {}
) -> void {
  auto registration = std::shared_ptr<mln::core::RuntimeEventRegistration>{};
  auto source = mln_runtime{MLN_HANDLE_NULL};
  {
    const std::scoped_lock state_lock(state->mutex);
    auto* runtime = state->runtime;
    if (
      !state->alive || runtime == nullptr ||
      !state->observed_regions.contains(region_id) ||
      !mln::core::event_selected(runtime->event_state->mask, type)
    ) {
      return;
    }
    registration = runtime->event_state->registration;
    source = runtime->self;
  }
  if (registration == nullptr) return;

  auto event = mln_runtime_event{
    .type = type,
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .source = source,
    .generation = 0,
    .code = 0,
    .payload_type = payload_type,
    .message = {},
    .payload = payload
  };
  registration->push(event, message);
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
    mln::FileSourceType::Database, resource_options_for_runtime(*runtime),
    mln::ClientOptions()
  );
  // MapLibre is built without RTTI. The registered FileSourceType::Database
  // factory always returns a DatabaseFileSource.
  auto database = std::static_pointer_cast<mln::DatabaseFileSource>(source);
  runtime->database_source = database;
  return runtime->database_source;
}

}  // namespace

auto lease_runtime(mln_runtime runtime) -> std::shared_ptr<RuntimeObject> {
  auto live = handle_table<RuntimeObject>().lease(runtime);
  if (live != nullptr && live->disposal_requested.load()) {
    static_cast<void>(report_handle_fault(
      HandleTraits<RuntimeObject>::kind, runtime, HandleFault::Stale
    ));
    return {};
  }
  return live;
}

auto run_runtime_command_task(RuntimeCommandTask& task) noexcept -> void {
  try {
    task.work(task);
  } catch (...) {
    // The command implementation reports its own terminal failure event.
  }
  task.clear_work(task);
  task.lease.reset();
}

auto submit_runtime_command_task(
  const std::shared_ptr<RuntimeObject>& runtime, RuntimeCommandTask& task
) -> mln_status {
  if (runtime == nullptr) {
    set_thread_error("runtime must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!runtime->control.acquire()) {
    set_thread_error("runtime is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  auto lease = ControlLease{&runtime->control};
  if (task.completion) {
    task.completion->set_submission_lease(std::move(lease));
    task.completion->accept();
  } else {
    task.lease = std::move(lease);
  }
  try {
    runtime->executor.enqueue(task);
  } catch (...) {
    if (task.completion) task.completion->revoke();
    set_thread_error("runtime command submission failed");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto submit_runtime_operation(
  const std::shared_ptr<RuntimeObject>& runtime,
  const std::shared_ptr<OperationObject>& operation,
  std::function<void()> function, const std::shared_ptr<Completion>& completion
) -> mln_status {
  if (runtime == nullptr || operation == nullptr || completion == nullptr) {
    set_thread_error("runtime, operation, and completion must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!runtime->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  completion->set_submission_lease(ControlLease{&runtime->control});
  completion->accept();
  try {
    runtime->executor.invoke(
      [operation, function = std::move(function)]() mutable -> void {
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
    completion->revoke();
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

struct RuntimeCreationGuard {
  RuntimeCreationGuard() = default;
  RuntimeCreationGuard(const RuntimeCreationGuard&) = delete;
  RuntimeCreationGuard(RuntimeCreationGuard&&) = delete;
  auto operator=(const RuntimeCreationGuard&) -> RuntimeCreationGuard& = delete;
  auto operator=(RuntimeCreationGuard&&) -> RuntimeCreationGuard& = delete;

  ~RuntimeCreationGuard() {
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

auto create_runtime(
  const mln_runtime_options* options, mln_runtime* out_runtime
) -> mln_status {
  if (out_runtime == nullptr || *out_runtime != MLN_HANDLE_NULL) {
    set_thread_error("out_runtime must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto options_status = validate_runtime_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  // Creation blocks until the executor's worker starts, which on the browser
  // main thread could wait forever.
  if (on_browser_main_thread()) {
    set_thread_error(
      "runtimes cannot be created on the browser main thread, which cannot "
      "block; create them on a Web Worker"
    );
    return MLN_STATUS_WRONG_THREAD;
  }
  if (
    !validate_c_string_view(options->asset_path, "asset_path") ||
    !validate_c_string_view(options->cache_path, "cache_path")
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  watch_process_exit();

  RuntimeCreationGuard result;
  result.runtime = std::make_shared<RuntimeObject>();
  auto asset_path = string_from_view(options->asset_path);
  auto cache_path = string_from_view(options->cache_path);

  auto runtime = result.runtime;
  runtime->executor.start(
    [runtime, asset_path = std::move(asset_path),
     cache_path = std::move(cache_path)]() mutable -> void {
      runtime->event_state = std::make_shared<RuntimeEventState>();
      runtime->event_state->mask.store(
        MLN_RUNTIME_EVENT_FLAG_ALL, std::memory_order_relaxed
      );
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
  const auto handle = handle_table<RuntimeObject>().insert(runtime);
  runtime->self = handle;
  bind_platform_context(runtime->platform_context, handle);
  result.published = true;
  *out_runtime = handle;
  return MLN_STATUS_OK;
}

namespace {

// Swaps one callback registration under its own lock and destroys the retired
// one after the lock is released, so a host's release_user_data never runs with
// that lock held. Taking the lock waits for the callbacks already running.
template <typename State, typename Registration>
auto replace_registration(
  State& state, Registration State::* member,
  std::type_identity_t<Registration> next
) -> void {
  auto previous = Registration{};
  {
    auto lock = std::unique_lock(state.mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
      if constexpr (std::is_same_v<State, ResourceTransformState>) {
        mln::testing::hit(mln::testing::SyncPoint::ResourceTransformExclusive);
      } else if constexpr (std::is_same_v<State, ResourceProviderState>) {
        mln::testing::hit(mln::testing::SyncPoint::ResourceProviderExclusive);
      }
      lock.lock();
    }
    previous = std::exchange(state.*member, std::move(next));
  }
}

// A registration change is a runtime command: a null completion discards its
// outcome, and a discarded failure is logged. A runtime has no snapshot, so
// the completion reports no generation.
template <typename Mutation>
auto execute_resource_configuration_command(
  const std::shared_ptr<Completion>& completion, Mutation&& mutation
) noexcept -> void {
  try {
    std::invoke(std::forward<Mutation>(mutation));
    complete_command(
      completion, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, {}, {}
    );
  } catch (const std::exception& exception) {
    complete_command(
      completion, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_NATIVE_ERROR, {},
      exception.what()
    );
  } catch (...) {
    complete_command(
      completion, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_NATIVE_ERROR, {},
      "resource configuration command failed"
    );
  }
}

template <typename Mutation>
auto submit_resource_configuration_command(
  const std::shared_ptr<RuntimeObject>& runtime,
  const mln_completion* descriptor, Mutation&& mutation
) -> mln_status {
  auto completion = std::shared_ptr<Completion>{};
  if (
    const auto made = make_command_completion(descriptor, completion);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  return submit_runtime_command(
    runtime,
    [completion, mutation = std::forward<Mutation>(mutation)]() mutable {
      execute_resource_configuration_command(completion, std::move(mutation));
    },
    completion
  );
}

}  // namespace

namespace {

// Roll back ownership on every rejected exit, including allocation exceptions
// while constructing or submitting the command. The caller keeps local storage
// until acceptance; the context owns it after commit.
class RegistrationTransfer {
 public:
  explicit RegistrationTransfer(RuntimeCallbackContext& context) noexcept
      : context_(context) {
    context_.transfer_to_runtime();
  }
  ~RegistrationTransfer() {
    if (!accepted_) context_.return_to_caller();
  }
  void accept() noexcept { accepted_ = true; }

 private:
  RuntimeCallbackContext& context_;
  bool accepted_ = false;
};

auto delete_header_rules(void* table) noexcept -> void {
  delete static_cast<HeaderRuleTable*>(table);
}

// Queues a copy of the URL with its request handle. A request the queue
// refuses keeps its URL.
auto queue_resource_transform(
  void* registration, mln_resource_kind kind, mln_buffer_view url,
  mln_resource_transform_request request
) noexcept -> void {
  const auto arguments = mln_resource_transform_delivery{
    .kind = kind,
    .url = url,
    .request = request,
  };
  if (!static_cast<const mln::delivery::QueuedRegistration*>(registration)
         ->push<MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM>(arguments)) {
    release_resource_transform_request(request);
  }
}

}  // namespace

auto set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform,
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (transform == nullptr) {
    set_thread_error("resource transform must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (transform->size < sizeof(mln_resource_transform)) {
    set_thread_error("mln_resource_transform.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto queue = std::shared_ptr<mln::delivery::DeliveryQueueState>{};
  const auto queue_status = mln::delivery::resolve_registration_queue(
    "resource transform", transform->queue, transform->callback != nullptr,
    transform->release_user_data != nullptr, queue
  );
  if (queue_status != MLN_STATUS_OK) return queue_status;

  auto queued = queue == nullptr
                  ? std::unique_ptr<mln::delivery::QueuedRegistration>{}
                  : std::make_unique<mln::delivery::QueuedRegistration>(
                      std::move(queue), transform->user_data
                    );
  const auto context =
    queued != nullptr
      ? std::make_shared<RuntimeCallbackContext>(
          queued.get(), &mln::delivery::QueuedRegistration::retire
        )
      : std::make_shared<RuntimeCallbackContext>(
          transform->user_data, transform->release_user_data
        );
  const auto registration = std::make_shared<ResourceTransformRegistration>(
    ResourceTransformRegistration{
      .callback =
        queued != nullptr ? &queue_resource_transform : transform->callback,
      .context = context,
    }
  );
  auto transfer = RegistrationTransfer{*context};
  const auto state = live->resource_transform_state;
  const auto status = submit_resource_configuration_command(
    live, completion, [state, registration]() -> void {
      replace_registration(
        *state, &ResourceTransformState::registration, registration
      );
    }
  );
  if (status == MLN_STATUS_OK) {
    static_cast<void>(queued.release());
    transfer.accept();
  }
  return status;
}

auto clear_resource_transform(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  const auto state = live->resource_transform_state;
  return submit_resource_configuration_command(
    live, completion, [state]() -> void {
      replace_registration(
        *state, &ResourceTransformState::registration, nullptr
      );
    }
  );
}

auto set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (transform == nullptr) {
    set_thread_error("HTTP header transform must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (transform->size < sizeof(mln_http_header_transform)) {
    set_thread_error("mln_http_header_transform.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto rules = std::unique_ptr<HeaderRuleTable>{};
  if (transform->callback == nullptr) {
    if (transform->rule_count == 0) {
      set_thread_error(
        "a transform requires a callback or rules; use the clear function"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (
      transform->user_data != nullptr || transform->release_user_data != nullptr
    ) {
      set_thread_error(
        "a rules HTTP header transform must not carry user_data or "
        "release_user_data"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    rules = std::make_unique<HeaderRuleTable>();
    const auto copied =
      copy_header_rules(transform->rules, transform->rule_count, *rules);
    if (copied != MLN_STATUS_OK) return copied;
  } else if (transform->rule_count != 0) {
    set_thread_error("an HTTP header transform callback takes no rules");
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

  const auto context = rules != nullptr
                         ? std::make_shared<RuntimeCallbackContext>(
                             rules.get(), &delete_header_rules
                           )
                         : std::make_shared<RuntimeCallbackContext>(
                             transform->user_data, transform->release_user_data
                           );
  const auto registration = std::make_shared<HttpHeaderTransformRegistration>(
    HttpHeaderTransformRegistration{
      .callback =
        rules != nullptr ? &header_rules_callback : transform->callback,
      .context = context,
    }
  );
  auto transfer = RegistrationTransfer{*context};
  const auto state = live->http_header_transform_state;
  const auto status = submit_resource_configuration_command(
    live, completion, [state, registration]() -> void {
      replace_registration(
        *state, &HttpHeaderTransformState::registration, registration
      );
    }
  );
  if (status == MLN_STATUS_OK) {
    static_cast<void>(rules.release());
    transfer.accept();
  }
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
  if (
    !validate_c_string_view(
      {.data = name, .size = name_size}, "HTTP header name"
    ) ||
    !validate_c_string_view(
      {.data = value, .size = value_size}, "HTTP header value"
    )
  ) {
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
  mln_http_header_transform_response* response, mln_buffer_view name,
  mln_buffer_view value
) -> mln_status {
  if (response == nullptr) {
    set_thread_error("HTTP header transform response must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (response->size < sizeof(mln_http_header_transform_response)) {
    set_thread_error("mln_http_header_transform_response.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto validation = validate_http_header(
    static_cast<const char*>(name.data), name.size,
    static_cast<const char*>(value.data), value.size
  );
  if (validation != MLN_STATUS_OK) {
    return validation;
  }
  const auto name_view = string_view_from_view(name);

  auto* const headers = static_cast<HttpHeaders*>(response->context);
  if (headers == nullptr) {
    set_thread_error(
      "HTTP headers can only be set during an HTTP header "
      "transform callback"
    );
    return MLN_STATUS_INVALID_STATE;
  }

  auto existing =
    std::ranges::find_if(*headers, [&](const HttpHeader& header) -> bool {
      return http_header_names_equal(header.first, name_view);
    });
  auto copied = HttpHeader{
    std::string{name_view},
    value.size == 0 ? std::string{} : string_from_view(value),
  };
  if (existing == headers->end()) {
    headers->push_back(std::move(copied));
  } else {
    *existing = std::move(copied);
  }
  return MLN_STATUS_OK;
}

auto clear_http_header_transform(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  const auto state = live->http_header_transform_state;
  return submit_resource_configuration_command(
    live, completion, [state]() -> void {
      replace_registration(
        *state, &HttpHeaderTransformState::registration, nullptr
      );
    }
  );
}

namespace {

using OfflinePresent =
  void (*)(const std::shared_ptr<Completion>& completion, std::any result);

template <auto Function>
auto present_nothing(const std::shared_ptr<Completion>& completion, std::any)
  -> void {
  valueless_completion<Function>().complete(completion);
}

template <auto Function>
auto present_regions(
  const std::shared_ptr<Completion>& completion, std::any result
) -> void {
  using Value = CompletionValue<Function>;
  static_assert(std::is_same_v<typename Value::Type, mln_offline_region_info>);
  if constexpr (Value::array) {
    auto value =
      std::any_cast<std::vector<OfflineRegionData>>(std::move(result));
    completion->resolve([value =
                           std::move(value)](const CompletionSink& descriptor) {
      auto info = std::vector<mln_offline_region_info>(value.size());
      for (size_t index = 0; index < value.size(); ++index) {
        if (fill_region_info(value[index], &info[index]) != MLN_STATUS_OK) {
          deliver_failure(
            descriptor, MLN_STATUS_NATIVE_ERROR, thread_last_error_message()
          );
          return;
        }
      }
      Value::deliver(descriptor, info);
    });
  } else {
    auto value = std::optional<OfflineRegionData>{};
    if constexpr (Value::nullable) {
      value =
        std::any_cast<std::optional<OfflineRegionData>>(std::move(result));
      if (!value) {
        completion->resolve([](const CompletionSink& descriptor) {
          Value::deliver_absent(descriptor);
        });
        return;
      }
    } else {
      value = std::any_cast<OfflineRegionData>(std::move(result));
    }
    completion->resolve(
      [value = std::move(*value)](const CompletionSink& descriptor) {
        auto info = mln_offline_region_info{};
        if (fill_region_info(value, &info) != MLN_STATUS_OK) {
          deliver_failure(
            descriptor, MLN_STATUS_NATIVE_ERROR, thread_last_error_message()
          );
          return;
        }
        Value::deliver(descriptor, info);
      }
    );
  }
}

auto present_region_status(
  const std::shared_ptr<Completion>& completion, std::any result
) -> void {
  CompletionValue<&mln_runtime_get_offline_region_status>::complete(
    completion, std::any_cast<mln_offline_region_status>(std::move(result))
  );
}

auto offline_operation_result_callback(
  const std::shared_ptr<Completion>& completion, OfflinePresent present
) -> OperationObject::ResultCallback {
  return [completion, present](
           mln_status status, std::string diagnostic, std::any result
         ) mutable {
    if (status != MLN_STATUS_OK) {
      complete_failure(completion, status, std::move(diagnostic));
      return;
    }
    try {
      present(completion, std::move(result));
    } catch (...) {
      complete_failure(
        completion, MLN_STATUS_NATIVE_ERROR,
        "offline operation produced an invalid result"
      );
    }
  };
}

// Accepts one offline operation on the calling thread and runs its native call
// on the runtime worker, which owns the database file source. No caller waits
// for the worker, so a completion delivered on the worker may submit further
// runtime work.
template <typename Schedule>
auto submit_offline_operation(
  const std::shared_ptr<RuntimeObject>& runtime, OfflinePresent present,
  const mln_completion* descriptor, Schedule schedule
) -> mln_status {
  auto completion = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(descriptor, completion);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  auto state = std::make_shared<OperationObject>(
    offline_operation_result_callback(completion, present)
  );
  const auto status = submit_runtime_operation(
    runtime, state,
    [runtime, state, schedule = std::move(schedule)]() mutable -> void {
      auto database = database_source_for_runtime(runtime.get());
      if (database == nullptr) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, "database file source is unavailable",
          std::any{std::monostate{}}
        );
        return;
      }
      try {
        std::invoke(std::move(schedule), state, database);
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR,
          exception_message(
            std::current_exception(), "offline operation submission failed"
          ),
          std::any{std::monostate{}}
        );
      }
    },
    completion
  );
  return status;
}

using OfflineOperationState = std::shared_ptr<OperationObject>;
using OfflineDatabase = std::shared_ptr<mln::DatabaseFileSource>;

}  // namespace

auto run_ambient_cache_operation_start(
  mln_runtime runtime, mln_ambient_cache_operation operation,
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
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

  return submit_offline_operation(
    live, present_nothing<&mln_runtime_run_ambient_cache_operation>, completion,
    [operation](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
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
  mln_runtime runtime, std::uint64_t size, const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  return submit_offline_operation(
    live, present_nothing<&mln_runtime_set_maximum_ambient_cache_size>,
    completion,
    [size](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
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
  const uint8_t* metadata, size_t metadata_size,
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  const auto definition_status = validate_offline_region_definition(definition);
  if (definition_status != MLN_STATUS_OK) {
    return definition_status;
  }
  if (metadata == nullptr && metadata_size != 0) {
    set_thread_error("offline region metadata must not be null when non-empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto native_definition = to_native_offline_region_definition(*definition);
  auto native_metadata = mln::OfflineRegionMetadata{};
  if (metadata_size != 0) {
    native_metadata.resize(metadata_size);
    std::memcpy(native_metadata.data(), metadata, metadata_size);
  }
  return submit_offline_operation(
    live, present_regions<&mln_runtime_create_offline_region>, completion,
    [native_definition = std::move(native_definition),
     native_metadata = std::move(native_metadata)](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->createOfflineRegion(
        native_definition, native_metadata,
        [state](mln::expected<mln::OfflineRegion, std::exception_ptr> result)
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
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  return submit_offline_operation(
    live, present_regions<&mln_runtime_get_offline_region>, completion,
    [region_id](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->getOfflineRegion(
        region_id,
        [state](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
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
  mln_runtime runtime, const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  return submit_offline_operation(
    live, present_regions<&mln_runtime_list_offline_regions>, completion,
    [](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->listOfflineRegions(
        [state](
          mln::expected<mln::OfflineRegions, std::exception_ptr> result
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

namespace {

// Opening the side database blocks, so it happens on the calling thread rather
// than on the runtime worker. A failure then reaches the caller as the call's
// own diagnostic.
auto validate_offline_side_database_path(mln_buffer_view side_database_path)
  -> mln_status {
  if (!validate_c_string_view(side_database_path, "side_database_path")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto path = mln::core::string_from_view(side_database_path);
  std::error_code filesystem_error;
  if (
    path.empty() || path.front() == ':' || path.compare(0, 5, "file:") == 0 ||
    !std::filesystem::is_regular_file(path, filesystem_error)
  ) {
    set_thread_error(
      "side database path must identify an existing readable database file"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto side_database =
    mapbox::sqlite::Database::tryOpen(path, mapbox::sqlite::ReadOnly);
  if (std::holds_alternative<mapbox::sqlite::Exception>(side_database)) {
    set_thread_error(
      "side database path must identify an existing readable database file"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace

auto offline_regions_merge_database_start(
  mln_runtime runtime, mln_buffer_view side_database_path,
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  const auto path_status =
    validate_offline_side_database_path(side_database_path);
  if (path_status != MLN_STATUS_OK) {
    return path_status;
  }

  return submit_offline_operation(
    live, present_regions<&mln_runtime_merge_offline_regions>, completion,
    [path = mln::core::string_from_view(side_database_path)](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->mergeOfflineRegions(
        path,
        [state](mln::expected<mln::OfflineRegions, std::exception_ptr> result)
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
  size_t metadata_size, const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (metadata == nullptr && metadata_size != 0) {
    set_thread_error("offline region metadata must not be null when non-empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto native_metadata = mln::OfflineRegionMetadata{};
  if (metadata_size != 0) {
    native_metadata.resize(metadata_size);
    std::memcpy(native_metadata.data(), metadata, metadata_size);
  }
  return submit_offline_operation(
    live, present_regions<&mln_runtime_update_offline_region_metadata>,
    completion,
    [region_id, native_metadata = std::move(native_metadata)](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->updateOfflineMetadata(
        region_id, native_metadata,
        [database, state, region_id](
          mln::expected<mln::OfflineRegionMetadata, std::exception_ptr> result
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
              mln::expected<
                std::optional<mln::OfflineRegion>, std::exception_ptr>
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
                  state, MLN_STATUS_NOT_FOUND, "offline region not found"
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
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  return submit_offline_operation(
    live, present_region_status, completion,
    [region_id](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
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
              state, MLN_STATUS_NOT_FOUND, "offline region not found"
            );
            return;
          }
          database->getOfflineRegionStatus(
            region.value(),
            [state](
              mln::expected<mln::OfflineRegionStatus, std::exception_ptr> result
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
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  return submit_offline_operation(
    live, present_nothing<&mln_runtime_set_offline_region_observed>, completion,
    [region_id, observed, offline_event_state = live->offline_event_state](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, region_id, observed, offline_event_state](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
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
              state, MLN_STATUS_NOT_FOUND, "offline region not found"
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
            if (observed) {
              offline_event_state->observed_regions.insert(region_id);
            } else {
              offline_event_state->observed_regions.erase(region_id);
            }
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
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  const auto native_state = to_native_download_state(request.state);
  if (!native_state) {
    set_thread_error("offline region download state is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return submit_offline_operation(
    live, present_nothing<&mln_runtime_set_offline_region_download_state>,
    completion,
    [region_id = request.region_id, download_state = *native_state](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, download_state](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
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
              state, MLN_STATUS_NOT_FOUND, "offline region not found"
            );
            return;
          }
          database->setOfflineRegionDownloadState(
            region.value(), download_state
          );
          complete_offline_operation(state, MLN_STATUS_OK, std::monostate{});
        }
      );
    }
  );
}

auto offline_region_invalidate_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  return submit_offline_operation(
    live, present_nothing<&mln_runtime_invalidate_offline_region>, completion,
    [region_id](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
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
              state, MLN_STATUS_NOT_FOUND, "offline region not found"
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
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  return submit_offline_operation(
    live, present_nothing<&mln_runtime_delete_offline_region>, completion,
    [region_id, offline_event_state = live->offline_event_state](
      const OfflineOperationState& state, const OfflineDatabase& database
    ) -> void {
      database->getOfflineRegion(
        region_id,
        [database, state, region_id, offline_event_state](
          mln::expected<std::optional<mln::OfflineRegion>, std::exception_ptr>
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
              state, MLN_STATUS_NOT_FOUND, "offline region not found"
            );
            return;
          }
          {
            const std::scoped_lock event_state_lock(offline_event_state->mutex);
            if (
              offline_event_state->alive &&
              offline_event_state->runtime != nullptr
            ) {
              offline_event_state->observed_regions.erase(region_id);
            }
          }
          database->setOfflineRegionObserver(region.value(), nullptr);
          database->setOfflineRegionDownloadState(
            region.value(), mln::OfflineRegionDownloadState::Inactive
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

namespace {

// Queues a copy of the request with its handle. A request the queue refuses
// fails, unless the process is exiting, which passes it through.
auto queue_provider_request(
  void* registration, const mln_resource_request_info* request,
  mln_resource_request handle
) noexcept -> void {
  const auto arguments = mln_resource_request_delivery{
    .request = request,
    .handle = handle,
  };
  if (
    static_cast<const mln::delivery::QueuedRegistration*>(registration)
      ->push<MLN_DELIVERY_TYPE_RESOURCE_REQUEST>(arguments)
  ) {
    return;
  }
  if (process_exiting()) {
    try {
      if (pass_through_resource_request(handle) == MLN_STATUS_OK) return;
    } catch (...) {
    }
  }
  release_resource_request(handle);
}

}  // namespace

auto set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider,
  const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (provider == nullptr) {
    set_thread_error("provider must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (provider->size < sizeof(mln_resource_provider)) {
    set_thread_error("mln_resource_provider.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto queue = std::shared_ptr<mln::delivery::DeliveryQueueState>{};
  const auto queue_status = mln::delivery::resolve_registration_queue(
    "provider", provider->queue, provider->callback != nullptr,
    provider->release_user_data != nullptr, queue
  );
  if (queue_status != MLN_STATUS_OK) return queue_status;

  auto routes = std::make_shared<RouteTable>();
  const auto routes_status =
    copy_routes(provider->routes, provider->route_count, *routes);
  if (routes_status != MLN_STATUS_OK) return routes_status;

  auto registration = std::unique_ptr<mln::delivery::QueuedRegistration>{};
  if (queue != nullptr) {
    registration = std::make_unique<mln::delivery::QueuedRegistration>(
      std::move(queue), provider->user_data
    );
  }
  const auto context =
    registration != nullptr
      ? std::make_shared<RuntimeCallbackContext>(
          registration.get(), &mln::delivery::QueuedRegistration::retire
        )
      : std::make_shared<RuntimeCallbackContext>(
          provider->user_data, provider->release_user_data
        );
  const auto copied = ResourceProvider{
    .callback =
      registration != nullptr ? &queue_provider_request : provider->callback,
    .context = context,
    .routes = routes->routes.empty() ? nullptr : std::move(routes),
  };
  auto transfer = RegistrationTransfer{*context};
  const auto state = live->resource_provider_state;
  const auto status = submit_resource_configuration_command(
    live, completion, [state, copied]() -> void {
      replace_registration(*state, &ResourceProviderState::provider, copied);
    }
  );
  if (status == MLN_STATUS_OK) {
    static_cast<void>(registration.release());
    transfer.accept();
  }
  return status;
}

auto clear_resource_provider(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();

  const auto state = live->resource_provider_state;
  return submit_resource_configuration_command(
    live, completion, [state]() -> void {
      replace_registration(
        *state, &ResourceProviderState::provider, ResourceProvider{}
      );
    }
  );
}

namespace {

auto release_runtime_reachable_state(RuntimeObject* runtime) -> void {
  replace_registration(
    *runtime->resource_transform_state, &ResourceTransformState::registration,
    nullptr
  );
  replace_registration(
    *runtime->http_header_transform_state,
    &HttpHeaderTransformState::registration, nullptr
  );
  replace_registration(
    *runtime->resource_provider_state, &ResourceProviderState::provider,
    ResourceProvider{}
  );
  {
    const std::scoped_lock state_lock(runtime->offline_event_state->mutex);
    runtime->offline_event_state->alive = false;
    runtime->offline_event_state->runtime = nullptr;
  }
  runtime->event_state->registration.reset();

  runtime->database_source.reset();
}

}  // namespace

namespace {

// Submission leases, children, and pending operations drain before this node
// reaches the worker. The shared finalizer joins that worker before the
// same node finishes retirement and reports the completion.
auto retire_runtime(
  std::shared_ptr<RuntimeObject> live, std::shared_ptr<Completion> completion
) noexcept -> void {
  live->release_completion = std::move(completion);
  live->disposal_owner = live;
  live->disposal_task.context = live.get();
  live->disposal_task.run = [](RetirementTask* task) noexcept {
    auto* retiring = static_cast<RuntimeObject*>(task->context);
    static_cast<void>(handle_table<RuntimeObject>().remove(retiring->self));
    unregister_platform_context(retiring->platform_context);
    release_runtime_reachable_state(retiring);
    task->run = [](RetirementTask* node) noexcept {
      auto* state = static_cast<RuntimeObject*>(node->context);
      state->executor.stop();
      auto owned = std::move(state->disposal_owner);
      auto release_completion = std::move(owned->release_completion);
      auto destroy = [&owned]() -> mln_status {
        owned.reset();
        return MLN_STATUS_OK;
      };
      static_cast<void>(mln::c_api::with_autorelease_pool(destroy));
      if (release_completion) {
        valueless_completion<&mln_runtime_release>().complete(
          release_completion
        );
      }
    };
    retiring->executor.retire(*task);
  };
  auto* retiring = live.get();
  live.reset();
  const auto drained = retiring->control.notify_when_drained(
    [](void* context) noexcept {
      auto& state = *static_cast<RuntimeObject*>(context);
      state.executor.invoke([task = &state.disposal_task] { task->run(task); });
    },
    retiring
  );
  if (!drained) {
    mln::testing::hit(mln::testing::SyncPoint::RuntimeRetirementDeferred);
  }
}

}  // namespace

auto dispose_runtime(mln_runtime runtime) -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  const auto status = live->control.begin_close(true);
  if (status != MLN_STATUS_OK) return status;
  live->disposal_requested = true;
  retire_runtime(std::move(live), nullptr);
  return MLN_STATUS_OK;
}

auto release_runtime(mln_runtime runtime, const mln_completion* completion)
  -> mln_status {
  auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  auto release_completion = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, release_completion);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  const auto release_status = live->control.begin_close();
  if (release_status != MLN_STATUS_OK) {
    return release_status;
  }
  live->disposal_requested = true;
  static_cast<void>(handle_table<RuntimeObject>().remove(runtime));
  // Accepting first keeps the completion on the finalizer even when the
  // runtime has already drained. Close rejects every later submission, so
  // retirement, which waits for the pending ones, reports after every earlier
  // accepted submission has finished.
  release_completion->accept();
  retire_runtime(std::move(live), std::move(release_completion));
  return MLN_STATUS_OK;
}

auto RuntimeEventRegistration::push(
  mln_runtime_event event, const std::string& message,
  std::vector<std::string> strings
) const -> void {
  if (queued != nullptr) {
    queued->push_event(event, message, std::move(strings));
  } else {
    if (process_exiting()) return;
    const auto text = delivery::event_message(message);
    // message is a std::string, so its view is NUL-terminated.
    event.message = mln_buffer_view{
      .data = text.empty() ? "" : text.data(), .size = text.size()
    };
    auto views = std::vector<mln_buffer_view>{};
    delivery::bind_event_strings(event, strings, views);
    // The callback borrows this span; bindings copy it before returning.
    const mln_event_batch_view events = {
      .size = sizeof(events),
      .event_size = sizeof(event),
      .events = &event,
      .event_count = 1,
    };
    callback(context->user_data(), &events);
  }
}

auto set_runtime_event_handler(
  mln_runtime runtime, const mln_runtime_event_handler* handler,
  const mln_completion* completion
) -> mln_status {
  const auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (handler == nullptr || handler->size < sizeof(*handler)) {
    set_thread_error("event handler must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto queue = std::shared_ptr<delivery::DeliveryQueueState>{};
  const auto validated = delivery::resolve_registration_queue(
    "event handler", handler->queue, handler->callback != nullptr,
    handler->release_user_data != nullptr, queue
  );
  if (validated != MLN_STATUS_OK) return validated;
  auto queued = queue == nullptr
                  ? std::unique_ptr<delivery::QueuedRegistration>{}
                  : std::make_unique<delivery::QueuedRegistration>(
                      queue, handler->user_data
                    );
  auto context = std::make_shared<RuntimeCallbackContext>(
    queued != nullptr ? queued.get() : handler->user_data,
    queued != nullptr ? &delivery::QueuedRegistration::retire
                      : handler->release_user_data
  );
  auto registration =
    std::make_shared<RuntimeEventRegistration>(RuntimeEventRegistration{
      .callback = handler->callback,
      .context = context,
      .queued = queued.get(),
    });
  auto transfer = RegistrationTransfer{*context};
  const auto status = submit_resource_configuration_command(
    live, completion, [live, registration]() {
      auto previous = std::shared_ptr<RuntimeEventRegistration>{};
      {
        const auto lock = std::scoped_lock{live->offline_event_state->mutex};
        previous = std::exchange(live->event_state->registration, registration);
      }
    }
  );
  if (status == MLN_STATUS_OK) {
    static_cast<void>(queued.release());
    transfer.accept();
  }
  return status;
}

auto clear_runtime_event_handler(
  mln_runtime runtime, const mln_completion* completion
) -> mln_status {
  const auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  return submit_resource_configuration_command(live, completion, [live]() {
    auto previous = std::shared_ptr<RuntimeEventRegistration>{};
    {
      const auto lock = std::scoped_lock{live->offline_event_state->mutex};
      previous = std::move(live->event_state->registration);
    }
  });
}

auto set_runtime_event_mask(mln_runtime runtime, uint64_t mask) -> mln_status {
  const auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!live->control.acquire()) {
    set_thread_error("runtime is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  auto control_lease = ControlLease{&live->control};
  if ((mask & ~static_cast<uint64_t>(MLN_RUNTIME_EVENT_FLAG_ALL)) != 0U) {
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
  if (out_mask == nullptr) {
    set_thread_error("out_mask must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!live->control.acquire()) {
    set_thread_error("runtime is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  auto control_lease = ControlLease{&live->control};
  *out_mask = live->event_state->mask.load(std::memory_order_relaxed);
  return MLN_STATUS_OK;
}

auto retain_runtime_map(mln_runtime runtime) -> mln_status {
  const auto live = lease_runtime(runtime);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!live->control.retain_child()) {
    return MLN_STATUS_INVALID_STATE;
  }
  return MLN_STATUS_OK;
}

auto release_runtime_map(mln_runtime runtime) noexcept -> void {
  const auto live = handle_table<RuntimeObject>().try_lease(runtime);
  if (live != nullptr) {
    live->control.release_child();
  }
}

auto runtime_run_loop(RuntimeObject* runtime) -> mln::util::RunLoop& {
  return *runtime->executor.run_loop();
}

auto resource_options_for_runtime(const RuntimeObject& runtime)
  -> mln::ResourceOptions {
  auto options = mln::ResourceOptions::Default();
#if defined(__ANDROID__)
  options.withAssetPath("/android_asset");
#endif
  options.withPlatformContext(runtime.platform_context);
  if (!runtime.asset_path.empty()) {
    options.withAssetPath(runtime.asset_path);
  }
  if (!runtime.cache_path.empty()) {
    options.withCachePath(runtime.cache_path);
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

  mln::testing::hit(mln::testing::SyncPoint::ResourceTransformLookup);
  const std::shared_lock transform_lock(state->mutex);
  return state->registration != nullptr;
}

auto invoke_resource_transform(
  void* platform_context, mln_resource_kind kind, const std::string& url,
  mln::ResourceTransform::FinishedCallback finished
) -> void {
  const auto state = lease_resource_transform_state(platform_context);
  if (state == nullptr) {
    finished(url);
    return;
  }

  // Same locking rule as `acquire_resource_provider_for_platform_context()`.
  mln::testing::hit(mln::testing::SyncPoint::ResourceTransformLookup);
  const std::shared_lock transform_lock(state->mutex);
  const auto registration = state->registration;
  if (registration == nullptr) {
    finished(url);
    return;
  }
  request_resource_transform(
    registration->callback, registration->context->user_data(), kind, url,
    std::move(finished)
  );
}

auto invoke_http_header_transform(
  void* platform_context, mln_resource_kind kind, const char* url
) noexcept -> HttpHeaders {
  const auto state = lease_http_header_transform_state(platform_context);
  if (state == nullptr || url == nullptr) {
    return {};
  }

  const std::shared_lock transform_lock(state->mutex);
  const auto registration = state->registration;
  if (registration == nullptr || process_exiting()) {
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
        registration->context->user_data(), kind,
        mln_buffer_view{.data = url, .size = std::strlen(url)}, &response
      ) != MLN_STATUS_OK
    ) {
      return {};
    }
    return headers;
  } catch (...) {
    return {};
  }
}

namespace {

auto map_event(
  mln_map map, uint64_t generation, mln_runtime_event_type type,
  mln_runtime_event_payload_type payload_type,
  const mln_runtime_event_payload& payload, uint32_t code
) -> mln_runtime_event {
  return mln_runtime_event{
    .type = type,
    .source_type = MLN_RUNTIME_EVENT_SOURCE_MAP,
    .source = map,
    .generation = generation,
    .code = code,
    .payload_type = payload_type,
    .message = {},
    .payload = payload
  };
}

}  // namespace

// Producers test their subscription mask before building an event.
auto push_runtime_map_event_payload(
  const std::shared_ptr<RuntimeEventState>& state, mln_map map,
  uint64_t generation, mln_runtime_event_type type,
  mln_runtime_event_payload_type payload_type,
  const mln_runtime_event_payload& payload, uint32_t code, std::string message,
  std::vector<std::string> strings
) -> void {
  if (state->registration == nullptr) return;
  state->registration->push(
    map_event(map, generation, type, payload_type, payload, code), message,
    std::move(strings)
  );
}

auto push_runtime_map_events(
  const std::shared_ptr<RuntimeEventState>& state, mln_map map,
  uint64_t generation, std::vector<HeldMapEvent>& events
) -> void {
  if (state->registration == nullptr) return;
  for (auto& held : events) {
    state->registration->push(
      map_event(
        map, generation, held.type, held.payload_type, held.payload, held.code
      ),
      held.message, std::move(held.strings)
    );
  }
}

}  // namespace mln::core
