#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <mln/style/sources/geojson_source.hpp>
#include <mln/util/immutable.hpp>

#include "execution/control_state.hpp"
#include "execution/retirement.hpp"
#include "handles/handle_table.hpp"
#include "map/feature_state.hpp"
#include "maplibre_native_c.h"

namespace mln {
class Map;
class MapProjection;
}  // namespace mln

namespace mln::core {

class CallbackSourceRegistry;
class Completion;
class HeadlessFrontend;
class HeadlessObserver;
struct MapEventState;
class OperationObject;
struct RuntimeObject;

// Worker-owned. The data that mln_map_set_geo_json_source_data() prepares for
// one GeoJSON source in the background, and the newest data that waits for it.
// Each completion is null when the caller passed none.
struct GeoJsonSourceUpdate {
  // Names the preparation in flight, or 0 when none is.
  uint64_t preparing = 0;
  std::shared_ptr<Completion> preparing_completion;
  bool waiting = false;
  std::string waiting_data;
  std::optional<mln::Immutable<mln::style::GeoJSONOptions>> waiting_options;
  std::shared_ptr<Completion> waiting_completion;
};

struct MapObject {
  ControlState control;
  RetirementTask disposal_task;
  std::shared_ptr<MapObject> disposal_owner;
  std::shared_ptr<Completion> release_completion;
  std::atomic_bool disposal_requested = false;
  // Set on the runtime worker once the native map exists. A map whose
  // creation failed keeps it false until it retires.
  std::atomic_bool created = false;
  // This map's own handle, so worker-side code that already holds the object
  // can name it in an event without resolving the table again.
  mln_map self = MLN_HANDLE_NULL;
  mln_runtime runtime = MLN_HANDLE_NULL;
  std::shared_ptr<RuntimeObject> runtime_state;
  // Runtime release waits until all map resources and worker pools retire.
  // Member order keeps the runtime alive until this lease releases.
  ControlLease runtime_cleanup_lease;
  mln_map_mode map_mode = MLN_MAP_MODE_CONTINUOUS;
  // Fixed at creation; applied to the renderer of each render session.
  bool tile_cache_enabled = true;
  // Fixed at creation; the style applies them to every style it loads.
  std::string fallback_font_faces;
  // Width and height follow resize on the runtime worker; scale_factor is
  // fixed at creation, so map_scale_factor() may read it from any thread.
  mln_logical_extent logical_extent{256, 256, 1.0};
  // Commands sent to the map, and those the runtime worker finished.
  std::atomic<uint64_t> commands_sent = 0;
  std::atomic<uint64_t> commands_applied = 0;
  // Written under the attached session's control mutex.
  std::atomic_bool commands_awaited = false;
  mutable std::mutex snapshot_mutex;
  mln_map_snapshot snapshot{};
  uint64_t next_snapshot_generation = 1;
  // Worker-owned invalidation; every command still publishes a generation.
  bool snapshot_config_dirty = true;
  bool snapshot_camera_dirty = true;
  bool still_image_request_pending = false;
  // Worker-owned IDs of the style images that the host set with keep_unused.
  // A successful style load clears it along with the images.
  std::unordered_set<std::string> kept_style_images;
  // Any-thread map-owned feature state; render sessions pull coalesced
  // snapshots and push diffs into their renderer.
  FeatureStateStore feature_state;
  std::shared_ptr<OperationObject> still_image_operation;
  // Releases the pending still request's submission lease exactly once.
  // Cancellation paths invoke it directly, because the mbgl callback that
  // otherwise releases it only fires when the mln::Map is destroyed — after
  // teardown already waits for submissions.
  std::function<void()> still_image_release_submission;
  // Worker-owned. Set while a style URL load waits for its style to parse,
  // with the completion of the command that started it, which is null when
  // the caller passed none. The parse or a load failure completes it, and a
  // newer style load or the map's release cancels it.
  bool style_load_pending = false;
  std::shared_ptr<Completion> style_load_completion;
  // Worker-owned, by source ID. A source's entry lasts while it prepares
  // data. Replacing the source's data another way, removing the source, a
  // style load, and the map's release cancel it.
  std::unordered_map<std::string, GeoJsonSourceUpdate> geojson_updates;
  uint64_t next_geojson_update = 1;
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
  // Whether the host disposed the attached session, which then still holds
  // the map until it retires. Guarded by the map handle table's mutex.
  bool render_target_session_disposed = false;
  // What the disposed session asked the map to run once the map closes, and
  // its argument. Guarded by the map handle table's mutex.
  void (*render_target_session_on_close)(mln_render_session) noexcept = nullptr;
  mln_render_session render_target_session_handle = MLN_HANDLE_NULL;

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

enum class CallbackSourceKind : uint8_t { CustomGeometry, CustomVector };

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
