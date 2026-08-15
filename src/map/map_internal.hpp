#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "execution/control_state.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mbgl {
class Map;
class MapProjection;
}  // namespace mbgl

namespace mln::core {

class CustomGeometrySourceRegistry;
class HeadlessFrontend;
class HeadlessObserver;
struct MapEventState;
class OperationObject;
struct RuntimeObject;

struct MapObject {
  ControlState control;
  mln_runtime runtime = MLN_HANDLE_NULL;
  std::shared_ptr<RuntimeObject> runtime_state;
  uint32_t map_mode = MLN_MAP_MODE_CONTINUOUS;
  mln_logical_extent logical_extent{256, 256, 1.0};
  mutable std::mutex snapshot_mutex;
  mln_map_snapshot snapshot{};
  uint64_t next_snapshot_generation = 1;
  std::atomic<uint64_t> latest_resize_command_id{0};
  bool still_image_request_pending = false;
  std::shared_ptr<OperationObject> still_image_operation;
  // Declared first so reverse-order destruction runs the release callbacks it
  // still owes after the mbgl::Map that could still reach a source is gone.
  std::shared_ptr<CustomGeometrySourceRegistry> custom_geometry_sources;
  // Declared before `observer` so reverse-order destruction retires the
  // observer, which holds its own reference, before this member is destroyed.
  std::shared_ptr<MapEventState> event_state;
  std::unique_ptr<HeadlessObserver> observer;
  std::unique_ptr<HeadlessFrontend> frontend;
  std::unique_ptr<mbgl::Map> map;
  // Guarded by the map handle table's mutex; a render session on another thread
  // clears it from map_detach_render_target_session().
  void* render_target_session = nullptr;

  MapObject() = default;
  MapObject(const MapObject&) = delete;
  MapObject(MapObject&&) = delete;
  auto operator=(const MapObject&) -> MapObject& = delete;
  auto operator=(MapObject&&) -> MapObject& = delete;

  ~MapObject();
};

template <>
struct HandleTraits<MapObject> {
  static constexpr auto kind = HandleKind::Map;
  static constexpr auto leasable = true;
};

struct MapProjectionObject {
  // Serializes every synchronous projection call, including close, so callers
  // may use one handle from any thread.
  std::mutex call_mutex;
  std::shared_ptr<MapObject> parent;
  std::shared_ptr<RuntimeObject> runtime_state;
  // Null once close has destroyed the projection; guarded by call_mutex.
  std::unique_ptr<mbgl::MapProjection> projection;

  MapProjectionObject() = default;
  MapProjectionObject(const MapProjectionObject&) = delete;
  MapProjectionObject(MapProjectionObject&&) = delete;
  auto operator=(const MapProjectionObject&) -> MapProjectionObject& = delete;
  auto operator=(MapProjectionObject&&) -> MapProjectionObject& = delete;
  ~MapProjectionObject();
};

template <>
struct HandleTraits<MapProjectionObject> {
  static constexpr auto kind = HandleKind::MapProjection;
  static constexpr auto leasable = true;
};

auto track_custom_geometry_source(
  MapObject& map, const std::string& source_id,
  mln_custom_geometry_source_release_callback release, void* user_data
) -> void;
auto untrack_custom_geometry_source(
  MapObject& map, const std::string& source_id
) noexcept -> void;
auto release_custom_geometry_source(
  MapObject& map, const std::string& source_id
) -> void;

}  // namespace mln::core
