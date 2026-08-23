#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "execution/control_state.hpp"
#include "handles/handle_table.hpp"
#include "map/feature_state.hpp"
#include "maplibre_native_c.h"

namespace mln {
class Map;
class MapProjection;
}  // namespace mln

namespace mln::core {

class CallbackSourceRegistry;
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
  std::atomic<uint64_t> latest_resize_submission{0};
  bool still_image_request_pending = false;
  // Any-thread map-owned feature state; render sessions pull coalesced
  // snapshots and push diffs into their renderer.
  FeatureStateStore feature_state;
  std::shared_ptr<OperationObject> still_image_operation;
  // Releases the pending still request's submission lease exactly once.
  // Cancellation paths invoke it directly, because the mbgl callback that
  // otherwise releases it only fires when the mln::Map is destroyed — after
  // teardown already waits for submissions.
  std::function<void()> still_image_release_submission;
  // Declared first so reverse-order destruction runs the release callbacks it
  // still owes after the mln::Map that could still reach a source is gone.
  std::shared_ptr<CallbackSourceRegistry> callback_sources;
  // Declared before `observer` so reverse-order destruction retires the
  // observer, which holds its own reference, before this member is destroyed.
  std::shared_ptr<MapEventState> event_state;
  std::unique_ptr<HeadlessObserver> observer;
  std::unique_ptr<HeadlessFrontend> frontend;
  std::unique_ptr<mln::Map> map;
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
  // Null once close has destroyed the projection; guarded by call_mutex.
  std::unique_ptr<mln::MapProjection> projection;

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

// The kinds of host-callback-backed sources the registry tracks.
enum class CallbackSourceKind : uint8_t { CustomGeometry, CustomMvtVector };

using CallbackSourceRelease = void (*)(void*);

auto track_callback_source(
  MapObject& map, const std::string& source_id, CallbackSourceKind kind,
  CallbackSourceRelease release, void* user_data
) -> void;
auto untrack_callback_source(
  MapObject& map, const std::string& source_id
) noexcept -> void;
auto release_callback_source(MapObject& map, const std::string& source_id)
  -> void;

}  // namespace mln::core
