#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <ratio>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <mln/actor/actor_ref.hpp>
#include <mln/actor/mailbox.hpp>
#include <mln/actor/scheduler.hpp>
#include <mln/gfx/rendering_stats.hpp>
#include <mln/map/bound_options.hpp>
#include <mln/map/camera.hpp>
#include <mln/map/map.hpp>
#include <mln/map/map_observer.hpp>
#include <mln/map/map_options.hpp>
#include <mln/map/map_projection.hpp>
#include <mln/map/mode.hpp>
#include <mln/map/projection_mode.hpp>
#include <mln/map/transform_state.hpp>
#include <mln/math/angles.hpp>
#include <mln/renderer/renderer_frontend.hpp>
#include <mln/renderer/renderer_observer.hpp>
#include <mln/renderer/update_parameters.hpp>
#include <mln/style/conversion.hpp>
#include <mln/style/conversion/geojson_options.hpp>  // IWYU pragma: keep
#include <mln/style/conversion/json.hpp>
#include <mln/style/conversion/layer.hpp>   // IWYU pragma: keep
#include <mln/style/conversion/light.hpp>   // IWYU pragma: keep
#include <mln/style/conversion/source.hpp>  // IWYU pragma: keep
#include <mln/style/conversion_impl.hpp>
#include <mln/style/image.hpp>
#include <mln/style/layer.hpp>
#include <mln/style/layers/color_relief_layer.hpp>
#include <mln/style/layers/hillshade_layer.hpp>
#include <mln/style/layers/location_indicator_layer.hpp>
#include <mln/style/light.hpp>
#include <mln/style/rapidjson_conversion.hpp>
#include <mln/style/source.hpp>
#include <mln/style/sources/custom_geometry_source.hpp>
#include <mln/style/sources/custom_vector_source.hpp>
#include <mln/style/sources/geojson_source.hpp>
#include <mln/style/sources/image_source.hpp>
#include <mln/style/sources/raster_dem_source.hpp>
#include <mln/style/sources/raster_source.hpp>
#include <mln/style/sources/vector_source.hpp>
#include <mln/style/style.hpp>
#include <mln/style/style_property.hpp>
#include <mln/style/transition_options.hpp>
#include <mln/style/types.hpp>
#include <mln/tile/tile_id.hpp>
#include <mln/tile/tile_operation.hpp>
#include <mln/util/chrono.hpp>
#include <mln/util/constants.hpp>
#include <mln/util/feature.hpp>
#include <mln/util/geo.hpp>
#include <mln/util/image.hpp>
#include <mln/util/immutable.hpp>
#include <mln/util/projection.hpp>
#include <mln/util/range.hpp>
#include <mln/util/size.hpp>
#include <mln/util/tileset.hpp>
#include <mln/util/vectors.hpp>

#include "map/map.hpp"

#include "completion/completion.hpp"
#include "delivery/queued_registration.hpp"
#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "map/logical_extent.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"
#include "style/style_value.hpp"
#include "testing/sync_point.hpp"

namespace {

constexpr auto default_map_width = uint32_t{256};
constexpr auto default_map_height = uint32_t{256};
constexpr double default_scale_factor = 1.0;

auto validate_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln_status;
auto validate_lat_lng_array(
  const mln_lat_lng* coordinates, size_t coordinate_count, bool allow_empty
) -> mln_status;
auto to_native_lat_lng(mln_lat_lng coordinate) -> mln::LatLng;
auto from_native_lat_lng(const mln::LatLng& coordinate) -> mln_lat_lng;
auto to_native_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln::LatLngBounds;
auto from_native_lat_lng_bounds(const mln::LatLngBounds& bounds)
  -> mln_lat_lng_bounds;

}  // namespace

namespace mln::core {

auto callback_source_matches(
  const mln::style::Source* source, CallbackSourceKind kind
) -> bool {
  if (source == nullptr) {
    return false;
  }
  switch (kind) {
    case CallbackSourceKind::CustomGeometry:
      return source->as<mln::style::CustomGeometrySource>() != nullptr;
    case CallbackSourceKind::CustomMvtVector:
      return source->as<mln::style::CustomVectorSource>() != nullptr;
  }
  return false;
}

// Holds the release callback owed for each tracked callback source. A host
// cannot see when a style load drops a source, so this layer tracks it.
//
// Every mutation runs on the runtime worker, which is the only thread that
// adds or removes sources and receives MapLibre style-load callbacks.
class CallbackSourceRegistry final {
 public:
  CallbackSourceRegistry() = default;

  // Runs when the owning map is destroyed, after the mln::Map that could still
  // call into the source is gone.
  ~CallbackSourceRegistry() { release_all(); }

  CallbackSourceRegistry(const CallbackSourceRegistry&) = delete;
  CallbackSourceRegistry(CallbackSourceRegistry&&) = delete;
  auto operator=(const CallbackSourceRegistry&)
    -> CallbackSourceRegistry& = delete;
  auto operator=(CallbackSourceRegistry&&) -> CallbackSourceRegistry& = delete;

  auto add(
    const std::string& source_id, CallbackSourceKind kind,
    CallbackSourceRelease release, void* user_data
  ) -> void {
    // An entry already under this ID belongs to a source that the style dropped
    // before reconciliation ran, so it still owes its release. Invoke it here
    // rather than letting the assignment below drop it, which keeps
    // exactly-once independent of when the style-loaded observer reconciles.
    if (const auto stale = entries_.find(source_id); stale != entries_.end()) {
      const auto owed = stale->second;
      entries_.erase(stale);
      invoke(owed);
    }
    if (release == nullptr) {
      return;
    }
    // The caller tracks before the style takes the source, so a throw here
    // leaves nothing committed and the caller still owns user_data. A failed
    // add owes no release.
    entries_.insert_or_assign(source_id, Entry{kind, release, user_data});
  }

  // Drops an entry without releasing it, for a source the style then rejected.
  // The caller still owns user_data on that path.
  auto untrack(const std::string& source_id) noexcept -> void {
    entries_.erase(source_id);
  }

  // The observer that reports style loads has no route to a style, so the
  // registry keeps the map that it belongs to until close detaches it.
  auto attach(mln::Map& map) noexcept -> void { map_ = &map; }

  auto detach() noexcept -> void { map_ = nullptr; }

  auto release(const std::string& source_id) -> void {
    const auto entry = entries_.find(source_id);
    if (entry == entries_.end()) {
      return;
    }
    const auto owed = entry->second;
    entries_.erase(entry);
    invoke(owed);
  }

  // Releases every tracked source the current style no longer holds. A style
  // document cannot declare a callback source, so a source of another type
  // under a tracked ID means the tracked source is gone.
  auto reconcile() -> void {
    if (map_ == nullptr || entries_.empty()) {
      return;
    }
    auto& style = map_->getStyle();
    auto owed = std::vector<Entry>{};
    for (auto entry = entries_.begin(); entry != entries_.end();) {
      auto* source = style.getSource(entry->first);
      if (callback_source_matches(source, entry->second.kind)) {
        ++entry;
        continue;
      }
      owed.push_back(entry->second);
      entry = entries_.erase(entry);
    }
    for (const auto& release : owed) {
      invoke(release);
    }
  }

  auto release_all() -> void {
    auto owed = std::move(entries_);
    entries_.clear();
    for (const auto& entry : owed) {
      invoke(entry.second);
    }
  }

 private:
  struct Entry {
    CallbackSourceKind kind = CallbackSourceKind::CustomGeometry;
    CallbackSourceRelease release = nullptr;
    void* user_data = nullptr;
  };

  static auto invoke(const Entry& entry) noexcept -> void {
    if (mln::core::process_exiting()) return;
    try {
      entry.release(entry.user_data);
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("callback source release threw");
    }
  }

  std::unordered_map<std::string, Entry> entries_;
  mln::Map* map_ = nullptr;
};

auto track_callback_source(
  MapObject& map, const std::string& source_id, CallbackSourceKind kind,
  CallbackSourceRelease release, void* user_data
) -> void {
  map.callback_sources->add(source_id, kind, release, user_data);
}

auto untrack_callback_source(
  MapObject& map, const std::string& source_id
) noexcept -> void {
  map.callback_sources->untrack(source_id);
}

auto release_callback_source(MapObject& map, const std::string& source_id)
  -> void {
  map.callback_sources->release(source_id);
}

}  // namespace mln::core

namespace {

using mln::core::CallbackSourceRegistry;

auto to_c_camera_change_mode(mln::MapObserver::CameraChangeMode mode)
  -> mln_camera_change_mode {
  switch (mode) {
    case mln::MapObserver::CameraChangeMode::Immediate:
      return MLN_CAMERA_CHANGE_MODE_IMMEDIATE;
    case mln::MapObserver::CameraChangeMode::Animated:
      return MLN_CAMERA_CHANGE_MODE_ANIMATED;
  }
  assert(false);
  return MLN_CAMERA_CHANGE_MODE_IMMEDIATE;
}

auto to_c_render_mode(mln::MapObserver::RenderMode mode) -> mln_render_mode {
  switch (mode) {
    case mln::MapObserver::RenderMode::Partial:
      return MLN_RENDER_MODE_PARTIAL;
    case mln::MapObserver::RenderMode::Full:
      return MLN_RENDER_MODE_FULL;
  }
  assert(false);
  return MLN_RENDER_MODE_PARTIAL;
}

auto to_c_rendering_stats(const mln::gfx::RenderingStats& stats)
  -> mln_rendering_stats {
  return mln_rendering_stats{
    .encoding_time = stats.encodingTime,
    .rendering_time = stats.renderingTime,
    .frame_count = stats.numFrames,
    .draw_call_count = stats.numDrawCalls,
    .total_draw_call_count = stats.totalDrawCalls
  };
}

auto render_frame_payload(const mln::MapObserver::RenderFrameStatus& status)
  -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.render_frame = mln_runtime_event_render_frame{
    .mode = to_c_render_mode(status.mode),
    .needs_repaint = status.needsRepaint,
    .placement_changed = status.placementChanged,
    // Native always attaches the frame's stats; a missing pointer reports
    // zeroed counters rather than reading through null.
    .stats = status.renderingStats != nullptr
               ? to_c_rendering_stats(*status.renderingStats)
               : mln_rendering_stats{}
  };
  return payload;
}

auto render_map_payload(mln::MapObserver::RenderMode mode)
  -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.render_map =
    mln_runtime_event_render_map{.mode = to_c_render_mode(mode)};
  return payload;
}

auto to_c_tile_operation(mln::TileOperation operation) -> mln_tile_operation {
  switch (operation) {
    case mln::TileOperation::RequestedFromCache:
      return MLN_TILE_OPERATION_REQUESTED_FROM_CACHE;
    case mln::TileOperation::RequestedFromNetwork:
      return MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK;
    case mln::TileOperation::LoadFromNetwork:
      return MLN_TILE_OPERATION_LOAD_FROM_NETWORK;
    case mln::TileOperation::LoadFromCache:
      return MLN_TILE_OPERATION_LOAD_FROM_CACHE;
    case mln::TileOperation::StartParse:
      return MLN_TILE_OPERATION_START_PARSE;
    case mln::TileOperation::EndParse:
      return MLN_TILE_OPERATION_END_PARSE;
    case mln::TileOperation::Error:
      return MLN_TILE_OPERATION_ERROR;
    case mln::TileOperation::Cancelled:
      return MLN_TILE_OPERATION_CANCELLED;
    case mln::TileOperation::NullOp:
      return MLN_TILE_OPERATION_NULL;
  }
  return MLN_TILE_OPERATION_NULL;
}

auto to_c_tile_id(const mln::OverscaledTileID& tile_id) -> mln_tile_id {
  return mln_tile_id{
    .overscaled_z = tile_id.overscaledZ,
    .wrap = tile_id.wrap,
    .canonical_z = tile_id.canonical.z,
    .canonical_x = tile_id.canonical.x,
    .canonical_y = tile_id.canonical.y
  };
}

auto tile_action_payload(
  mln::TileOperation operation, const mln::OverscaledTileID& tile_id
) -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.tile_action = mln_runtime_event_tile_action{
    .operation = to_c_tile_operation(operation),
    .tile_id = to_c_tile_id(tile_id)
  };
  return payload;
}

}  // namespace

namespace mln::core {

// The end handler that one camera command copied. Its callback runs at most
// once, and release_user_data runs after it. The submission transfers the
// handler before it commits and takes it back when the submission fails, so
// a rejected command runs neither. An accepted command that the runtime drops
// without ending reports itself closed when its last owner lets go. A queue
// handler's registration stands in for user_data, and a rejected command
// frees it without retiring it.
class CameraTransitionEnd final {
 public:
  explicit CameraTransitionEnd(
    const mln_camera_transition_handler& handler
  ) noexcept
      : handler_(handler) {}
  CameraTransitionEnd(const CameraTransitionEnd&) = delete;
  CameraTransitionEnd(CameraTransitionEnd&&) = delete;
  auto operator=(const CameraTransitionEnd&) -> CameraTransitionEnd& = delete;
  auto operator=(CameraTransitionEnd&&) -> CameraTransitionEnd& = delete;
  ~CameraTransitionEnd() { deliver(MLN_CAMERA_TRANSITION_OUTCOME_CLOSED, 0); }

  auto transfer_to_runtime() noexcept -> void {
    owned_.store(true, std::memory_order_release);
  }
  auto return_to_caller() noexcept -> void {
    owned_.store(false, std::memory_order_release);
  }

  auto deliver(
    mln_camera_transition_outcome outcome, uint64_t generation
  ) noexcept -> void {
    if (!owned_.exchange(false, std::memory_order_acq_rel)) return;
    if (process_exiting()) return;
    const auto end = mln_camera_transition_end{
      .size = sizeof(mln_camera_transition_end),
      .outcome = outcome,
      .generation = generation,
    };
    try {
      handler_.callback(handler_.user_data, &end);
    } catch (...) {
      // Host callbacks must not unwind through the C boundary.
    }
    if (handler_.release_user_data == nullptr || process_exiting()) return;
    try {
      handler_.release_user_data(handler_.user_data);
    } catch (...) {
    }
  }

  static auto queue_end(
    void* registration, const mln_camera_transition_end* end
  ) noexcept -> void {
    static_cast<mln::delivery::QueuedRegistration*>(registration)
      ->push_reserved<MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END>(
        mln_camera_transition_end_delivery{.end = end}
      );
  }

 private:
  mln_camera_transition_handler handler_;
  std::atomic_bool owned_{false};
};

namespace {

// Queues one event that the map raised on the runtime worker. Inside a map
// transaction the event waits for the generation that the transaction
// publishes, and `generation` is unused. Outside one, `generation` stamps the
// event, and zero drops it, because zero means the map is gone or its publish
// failed. Every map producer queues through here. Most run inside MapLibre
// callbacks, so an event that cannot be queued is dropped rather than thrown.
// The message is copied inside the guard for the same reason.
auto queue_map_event(
  MapEventState& events, uint64_t generation, mln_runtime_event_type type,
  mln_runtime_event_payload_type payload_type = MLN_RUNTIME_EVENT_PAYLOAD_NONE,
  const mln_runtime_event_payload& payload = zeroed_event_payload(),
  uint32_t code = 0, std::string_view message = {}
) noexcept -> void {
  try {
    if (events.in_transaction()) {
      events.held.push_back(
        HeldMapEvent{
          .type = type,
          .payload_type = payload_type,
          .payload = payload,
          .code = code,
          .message = std::string{message},
        }
      );
      return;
    }
    if (generation == 0) return;
    push_runtime_map_event_payload(
      events.runtime_events, events.map, generation, type, payload_type,
      payload, code, std::string{message}
    );
  } catch (...) {
    // MapLibre callbacks drop events that cannot be delivered.
  }
}

// Queues the events that a transaction held, all with the generation that it
// published, ahead of any event that the map raises afterwards. The held
// vector keeps its capacity for the next transaction.
auto queue_held_map_events(MapEventState& events, uint64_t generation) -> void {
  if (events.held.empty()) return;
  try {
    push_runtime_map_events(
      events.runtime_events, events.map, generation, events.held
    );
  } catch (...) {
    events.held.clear();
    throw;
  }
  events.held.clear();
}

// Records that the transitions of a camera command with an end handler ended.
// The end waits for the camera change that MapLibre raises next, or for the
// transaction around the change, to learn its generation. A command that a
// map close already reported records nothing.
auto end_camera_command(
  MapEventState& events, const std::shared_ptr<CameraTransitionEnd>& end,
  mln_camera_transition_outcome outcome
) noexcept -> void {
  auto& running = events.running_transition_ends;
  const auto found = std::find(running.begin(), running.end(), end);
  if (found == running.end()) return;
  running.erase(found);
  try {
    events.ended_transitions.push_back(
      EndedCameraTransition{.end = end, .outcome = outcome}
    );
  } catch (...) {
    // The handler must still run exactly once.
    end->deliver(outcome, events.published_generation);
  }
}

// Runs the handlers of the ended commands with `generation`. Callers run this
// only from a runtime-worker task, never from inside a MapLibre call.
auto deliver_ended_transitions(
  MapEventState& events, uint64_t generation
) noexcept -> void {
  auto ended = std::exchange(events.ended_transitions, {});
  for (const auto& item : ended) item.end->deliver(item.outcome, generation);
}

struct EndedTransitionDelivery {
  std::vector<EndedCameraTransition> ended;
  uint64_t generation;

  EndedTransitionDelivery(
    std::vector<EndedCameraTransition> items, uint64_t value
  )
      : ended(std::move(items)), generation(value) {}
  EndedTransitionDelivery(EndedTransitionDelivery&&) noexcept = default;
  ~EndedTransitionDelivery() { deliver(); }

  auto deliver() noexcept -> void {
    for (const auto& item : ended) item.end->deliver(item.outcome, generation);
    ended.clear();
  }
};

auto post_ended_transitions(MapEventState& events, uint64_t generation) noexcept
  -> void;

// Reports the camera commands that a closing map still runs as closed, after
// the commands that ended before the close.
auto close_camera_transitions(MapEventState& events) noexcept -> void {
  deliver_ended_transitions(events, events.published_generation);
  auto running = std::exchange(events.running_transition_ends, {});
  for (const auto& end : running) {
    end->deliver(MLN_CAMERA_TRANSITION_OUTCOME_CLOSED, 0);
  }
}

// Marks the transitions that a C API cancellation ends for as long as it
// runs, so their finish callbacks report them cancelled.
class TransitionCancellationScope final {
 public:
  TransitionCancellationScope(
    MapEventState& events, CameraTransitionCancellation cancellation
  ) noexcept
      : events_(events),
        previous_(std::exchange(events.cancelling_transitions, cancellation)) {}
  TransitionCancellationScope(const TransitionCancellationScope&) = delete;
  TransitionCancellationScope(TransitionCancellationScope&&) = delete;
  auto operator=(const TransitionCancellationScope&)
    -> TransitionCancellationScope& = delete;
  auto operator=(TransitionCancellationScope&&)
    -> TransitionCancellationScope& = delete;
  ~TransitionCancellationScope() { events_.cancelling_transitions = previous_; }

 private:
  MapEventState& events_;
  CameraTransitionCancellation previous_;
};

}  // namespace

// Every callback tests the map's subscription mask before it builds anything,
// so an unselected event allocates no payload, message, or queue node. An event
// that reports a change in map state takes a fresh generation, so the snapshot
// that a host reads for it already includes the change.
class HeadlessObserver final : public mln::MapObserver {
 public:
  HeadlessObserver(
    std::shared_ptr<mln::core::MapEventState> event_state,
    std::shared_ptr<CallbackSourceRegistry> callback_sources
  )
      : event_state_(std::move(event_state)),
        callback_sources_(std::move(callback_sources)) {}

  void onCameraWillChange(CameraChangeMode mode) override {
    if (const auto map = event_state_->owner.lock())
      map->snapshot_camera_dirty = true;
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE)) {
      return;
    }
    // The camera has not moved yet, so the latest snapshot is current.
    queue_current(
      MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, to_c_camera_change_mode(mode)
    );
  }

  void onCameraIsChanging() override {
    if (const auto map = event_state_->owner.lock())
      map->snapshot_camera_dirty = true;
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING)) {
      return;
    }
    queue_fresh(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING);
  }

  // MapLibre calls a transition's finish callback immediately before this, so
  // the commands that it ended take this change's generation. A transaction
  // runs their handlers when it publishes. Outside one, they run from a later
  // task, after this change's event is queued, whatever the mask selects.
  void onCameraDidChange(CameraChangeMode mode) override {
    if (const auto map = event_state_->owner.lock())
      map->snapshot_camera_dirty = true;
    auto& events = *event_state_;
    const auto did_change = selected(MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE);
    const auto ended =
      !events.in_transaction() && !events.ended_transitions.empty();
    if (!did_change && !ended) return;
    const auto generation = events.fresh_generation();
    if (did_change) {
      queue_map_event(
        events, generation, MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
        MLN_RUNTIME_EVENT_PAYLOAD_NONE, zeroed_event_payload(),
        to_c_camera_change_mode(mode)
      );
    }
    if (ended) post_ended_transitions(events, generation);
  }

  void onWillStartLoadingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED)) {
      return;
    }
    // Loading has changed no published state yet.
    queue_current(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED);
  }

  void onDidFinishLoadingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED)) {
      return;
    }
    queue_fresh(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED);
  }

  // The failure text is map state that both style setters read, so it is
  // recorded whatever the mask selects. A copy that fails leaves the text
  // empty, but the flag still tells the setters that the load failed.
  void onDidFailLoadingMap(
    mln::MapLoadError error, const std::string& message
  ) override {
    event_state_->style_load_failed = true;
    try {
      event_state_->style_load_failure = message;
    } catch (...) {
      event_state_->style_load_failure.clear();
    }
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_FAILED)) {
      return;
    }
    queue_fresh(
      MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, static_cast<uint32_t>(error),
      message
    );
  }

  // A style load can drop custom geometry sources that the previous style held.
  // The release callbacks this owes are map state, not an event, so the
  // reconciliation runs whatever the mask selects.
  void onDidFinishLoadingStyle() override {
    // The event is queued before the reconciliation, and the registry is held
    // by a local share across it, because reconcile() runs host release
    // callbacks. Nothing may touch this observer or its members after host code
    // runs: a callback that destroys its map destroys this observer with it.
    if (selected(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)) {
      queue_fresh(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED);
    }
    const auto sources = callback_sources_;
    sources->reconcile();
  }

  void onWillStartRenderingFrame() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED)) {
      return;
    }
    queue_current(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED);
  }

  void onDidFinishRenderingFrame(const RenderFrameStatus& status) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED)) {
      return;
    }
    queue_current(
      MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED, 0, {},
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME, render_frame_payload(status)
    );
  }

  void onWillStartRenderingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED)) {
      return;
    }
    queue_current(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED);
  }

  void onDidFinishRenderingMap(RenderMode mode) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED)) {
      return;
    }
    queue_current(
      MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED, 0, {},
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP, render_map_payload(mode)
    );
  }

  void onDidBecomeIdle() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_IDLE)) {
      return;
    }
    queue_fresh(MLN_RUNTIME_EVENT_MAP_IDLE);
  }

  void onStyleImageMissing(const std::string& image_id) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING)) {
      return;
    }
    queue_current(MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING, 0, image_id);
  }

  void onSourceChanged(mln::style::Source& source) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED)) {
      return;
    }
    queue_current(MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED, 0, source.getID());
  }

  void onTileAction(
    mln::TileOperation operation, const mln::OverscaledTileID& tile_id,
    const std::string& source_id
  ) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_TILE_ACTION)) {
      return;
    }
    queue_current(
      MLN_RUNTIME_EVENT_MAP_TILE_ACTION, 0, source_id,
      MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
      tile_action_payload(operation, tile_id)
    );
  }

  // The mask test precedes the try block, so a suppressed render error never
  // formats the exception text.
  void onRenderError(std::exception_ptr error) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR)) {
      return;
    }
    try {
      if (error) {
        std::rethrow_exception(error);
      }
      queue_current(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR);
    } catch (const std::exception& exception) {
      queue_current(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, 0, exception.what());
    } catch (...) {
      queue_current(
        MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, 0, "unknown render error"
      );
    }
  }

 private:
  [[nodiscard]] auto selected(mln_runtime_event_type type) const noexcept
    -> bool {
    return mln::core::event_selected(event_state_->mask, type);
  }

  auto queue_fresh(
    mln_runtime_event_type type, uint32_t code = 0,
    std::string_view message = {}
  ) noexcept -> void {
    auto& events = *event_state_;
    queue_map_event(
      events, events.fresh_generation(), type, MLN_RUNTIME_EVENT_PAYLOAD_NONE,
      zeroed_event_payload(), code, message
    );
  }

  auto queue_current(
    mln_runtime_event_type type, uint32_t code = 0,
    std::string_view message = {},
    mln_runtime_event_payload_type payload_type =
      MLN_RUNTIME_EVENT_PAYLOAD_NONE,
    const mln_runtime_event_payload& payload = zeroed_event_payload()
  ) noexcept -> void {
    auto& events = *event_state_;
    queue_map_event(
      events, events.published_generation, type, payload_type, payload, code,
      message
    );
  }

  std::shared_ptr<mln::core::MapEventState> event_state_;
  std::shared_ptr<CallbackSourceRegistry> callback_sources_;
};

}  // namespace mln::core

namespace {

using mln::core::HeadlessObserver;

// Keeps a pending still image from completing on a frame that rendered an
// update older than its request. MapLibre completes a still image after any
// fully loaded frame, but a render session re-renders the latest update on a
// forced demand, and a frame can start before the request's own update is
// published. Such a frame reaches mln::Map::Impl as a partial one, so the
// image waits for a frame that renders its request. Runs on the map's run
// loop, like the delegate.
class StillImageFrameFilter {
 public:
  using RenderMode = mln::RendererObserver::RenderMode;

  explicit StillImageFrameFilter(mln::RendererObserver& delegate)
      : delegate_(delegate) {}

  // The generation of the update the pending request published, or zero when
  // no request is pending.
  auto set_request_generation(uint64_t generation) noexcept -> void {
    request_generation_ = generation;
  }

  void finish_frame(
    RenderMode mode, bool repaint_needed, bool placement_changed,
    std::shared_ptr<mln::gfx::RenderingStats> stats,
    uint64_t rendered_generation
  ) {
    const auto held_back = rendered_generation < request_generation_;
    if (held_back) mode = RenderMode::Partial;
    delegate_.onDidFinishRenderingFrame(
      mode, repaint_needed, placement_changed, std::move(stats)
    );
    if (held_back) {
      mln::testing::hit(mln::testing::SyncPoint::StillImageFrameHeldBack);
    }
  }

 private:
  mln::RendererObserver& delegate_;
  uint64_t request_generation_ = 0;
};

// Delivers mln::RendererObserver callbacks on the map's run loop instead of on
// whichever thread rendered. The delegate is mln::Map::Impl, whose handlers
// must not run concurrently. Every callback therefore becomes a mailbox message
// submitted to the runtime worker. Forwarding is unconditional, preserving
// delivery order independently of the callback's native thread.
class ForwardingRendererObserver final : public mln::RendererObserver {
 public:
  ForwardingRendererObserver(
    mln::Scheduler& map_scheduler, mln::RendererObserver& delegate
  )
      : mailbox_(std::make_shared<mln::Mailbox>(map_scheduler)),
        delegate_(delegate, mailbox_),
        still_image_frames_(delegate),
        still_image_frames_ref_(still_image_frames_, mailbox_) {}

  ForwardingRendererObserver(const ForwardingRendererObserver&) = delete;
  auto operator=(const ForwardingRendererObserver&)
    -> ForwardingRendererObserver& = delete;
  ForwardingRendererObserver(ForwardingRendererObserver&&) = delete;
  auto operator=(ForwardingRendererObserver&&)
    -> ForwardingRendererObserver& = delete;

  ~ForwardingRendererObserver() override { mailbox_->close(); }

  // Waits out an in-flight receive and drops anything queued, so the delegate
  // can be torn down once this returns. Idempotent.
  auto close() -> void { mailbox_->close(); }

  // Called on the rendering thread before each render with the generation of
  // the update it renders.
  auto begin_render(uint64_t update_generation) noexcept -> void {
    rendering_generation_.store(update_generation, std::memory_order_relaxed);
  }

  // Called on the map's run loop.
  [[nodiscard]] auto still_image_frames() -> StillImageFrameFilter& {
    return still_image_frames_;
  }

  void onInvalidate() override {
    delegate_.invoke(&mln::RendererObserver::onInvalidate);
  }

  void onResourceError(std::exception_ptr error) override {
    delegate_.invoke(&mln::RendererObserver::onResourceError, error);
  }

  void onWillStartRenderingMap() override {
    delegate_.invoke(&mln::RendererObserver::onWillStartRenderingMap);
  }

  void onWillStartRenderingFrame() override {
    delegate_.invoke(&mln::RendererObserver::onWillStartRenderingFrame);
  }

  void onDidFinishRenderingFrame(
    RenderMode mode, bool repaint_needed, bool placement_changed,
    std::shared_ptr<mln::gfx::RenderingStats> stats
  ) override {
    // The name carries four overloads; mln::Map::Impl implements only this
    // one, and it schedules the next update from it. The filter shares the
    // delegate's mailbox, so the frame keeps its place among the callbacks.
    still_image_frames_ref_.invoke(
      &StillImageFrameFilter::finish_frame, mode, repaint_needed,
      placement_changed, std::move(stats),
      rendering_generation_.load(std::memory_order_relaxed)
    );
  }

  void onDidFinishRenderingMap() override {
    delegate_.invoke(&mln::RendererObserver::onDidFinishRenderingMap);
  }

  void onStyleImageMissing(
    const std::string& id, const StyleImageMissingCallback& done
  ) override {
    delegate_.invoke(&mln::RendererObserver::onStyleImageMissing, id, done);
  }

  void onRemoveUnusedStyleImages(const std::vector<std::string>& ids) override {
    delegate_.invoke(&mln::RendererObserver::onRemoveUnusedStyleImages, ids);
  }

  void onPreCompileShader(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onPreCompileShader, id, type, defines
    );
  }

  void onPostCompileShader(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onPostCompileShader, id, type, defines
    );
  }

  void onShaderCompileFailed(
    mln::shaders::BuiltIn id, mln::gfx::Backend::Type type,
    const std::string& defines
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onShaderCompileFailed, id, type, defines
    );
  }

  void onGlyphsLoaded(
    const mln::FontStack& stack, const mln::GlyphRange& range
  ) override {
    delegate_.invoke(&mln::RendererObserver::onGlyphsLoaded, stack, range);
  }

  void onGlyphsError(
    const mln::FontStack& stack, const mln::GlyphRange& range,
    std::exception_ptr error
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onGlyphsError, stack, range, error
    );
  }

  void onGlyphsRequested(
    const mln::FontStack& stack, const mln::GlyphRange& range
  ) override {
    delegate_.invoke(&mln::RendererObserver::onGlyphsRequested, stack, range);
  }

  void onTileAction(
    mln::TileOperation operation, const mln::OverscaledTileID& id,
    const std::string& source_id
  ) override {
    delegate_.invoke(
      &mln::RendererObserver::onTileAction, operation, id, source_id
    );
  }

  void onRenderError(std::exception_ptr error) override {
    delegate_.invoke(&mln::RendererObserver::onRenderError, error);
  }

 private:
  std::shared_ptr<mln::Mailbox> mailbox_;
  mln::ActorRef<mln::RendererObserver> delegate_;
  StillImageFrameFilter still_image_frames_;
  mln::ActorRef<StillImageFrameFilter> still_image_frames_ref_;
  std::atomic<uint64_t> rendering_generation_{0};
};

}  // namespace

namespace mln::core {

class HeadlessFrontend final : public mln::RendererFrontend {
 public:
  // The thread pool tag must be a default-constructed identity, unique per map.
  // SimpleIdentity::Empty pools every map's work into one bucket that
  // waitForEmpty() cannot wait on, because it remaps the empty tag to the
  // pool's own identity. The run loop comes in by reference because mbgl calls
  // setObserver() from the map constructor; it outlives the map.
  HeadlessFrontend(
    mln::util::RunLoop& run_loop,
    std::shared_ptr<mln::core::MapEventState> event_state,
    FeatureStateStore& feature_state
  )
      : run_loop_(run_loop),
        event_state_(std::move(event_state)),
        feature_state_(feature_state),
        thread_pool_(
          mln::Scheduler::GetBackground(), mln::util::SimpleIdentity{}
        ) {}

  void reset() override {
    const std::scoped_lock lock(latest_update_mutex_);
    latest_update_.reset();
    held_update_.reset();
    latest_feature_state_.reset();
    held_feature_state_.reset();
  }

  // mln::Map calls this once from its constructor on the runtime worker.
  void setObserver(mln::RendererObserver& observer) override {
    observer_ =
      std::make_unique<ForwardingRendererObserver>(run_loop_, observer);
  }

  // Runs on the runtime worker. An open map transaction or command group holds
  // the update. Each update is a complete snapshot of the map, so only the
  // newest held one is kept. Otherwise the update is stored at once, and its
  // session notice waits for the announcement task.
  void update(std::shared_ptr<mln::UpdateParameters> update) override {
    auto feature_state = feature_state_.snapshot();
    {
      const std::scoped_lock lock(latest_update_mutex_);
      if (event_state_->in_transaction() || command_group_depth_ > 0) {
        held_update_ = std::move(update);
        held_feature_state_ = std::move(feature_state);
        return;
      }
      latest_update_ = std::move(update);
      latest_feature_state_ = std::move(feature_state);
      ++latest_update_generation_;
    }
    schedule_announcement();
  }

  // Stores the held update when no command group remains open. A transaction
  // calls this before it publishes its snapshot, so the snapshot reports the
  // update.
  auto store_held_update() -> void {
    if (command_group_depth_ > 0) return;
    {
      const std::scoped_lock lock(latest_update_mutex_);
      if (held_update_ == nullptr) return;
      latest_update_ = std::move(held_update_);
      latest_feature_state_ = std::move(held_feature_state_);
      ++latest_update_generation_;
    }
    update_stored_ = true;
  }

  // Announces the update that the closing transaction stored to its session.
  auto release_held_update() -> void {
    if (!std::exchange(update_stored_, false)) return;
    schedule_announcement();
  }

  // Runs after a command burst, before the next non-command work item, so
  // queued commands reach the attached session as one notice and one frame. The
  // updates stored outside a transaction share the session notice.
  auto announce() noexcept -> void {
    if (!std::exchange(announcement_scheduled_, false)) return;
    auto generation = uint64_t{0};
    auto notify_session = std::function<void(uint64_t)>{};
    {
      const std::scoped_lock lock(latest_update_mutex_);
      generation = latest_update_generation_;
      notify_session = session_notices_.update;
    }
    // Publish asynchronous camera and loading changes before the session sees
    // their update, including when the host selects no events.
    event_state_->fresh_generation();
    if (notify_session) notify_session(generation);
  }

  // Command groups nest, and only map commands open or end one.
  auto begin_command_group() -> void { ++command_group_depth_; }
  auto end_command_group() -> bool {
    if (command_group_depth_ == 0) return false;
    --command_group_depth_;
    return true;
  }
  [[nodiscard]] auto command_group_open() const -> bool {
    return command_group_depth_ > 0;
  }

  [[nodiscard]] auto latest_update() const
    -> std::shared_ptr<mln::UpdateParameters> {
    const std::scoped_lock lock(latest_update_mutex_);
    return latest_update_;
  }
  [[nodiscard]] auto latest_update_snapshot() const -> RenderUpdateSnapshot {
    const std::scoped_lock lock(latest_update_mutex_);
    return {latest_update_, latest_feature_state_, latest_update_generation_};
  }
  auto set_session_notices(RenderSessionNotices notices) -> void {
    const std::scoped_lock lock(latest_update_mutex_);
    session_notices_ = std::move(notices);
  }

  [[nodiscard]] auto latest_update_generation() const -> uint64_t {
    const std::scoped_lock lock(latest_update_mutex_);
    return latest_update_generation_;
  }

  // The generation of the first update that includes the map's current state.
  // While a command group holds an update, nothing else stores one, so the
  // outermost group end stores it as the next generation.
  [[nodiscard]] auto pending_update_generation() const -> uint64_t {
    const std::scoped_lock lock(latest_update_mutex_);
    return latest_update_generation_ + (held_update_ != nullptr ? 1 : 0);
  }

  auto run_render_jobs() -> void { thread_pool_.runRenderJobs(); }

  // Drains queued and running worker jobs without closing the queue; later
  // work re-creates the pool bucket, and shutdown_thread_pool() still runs at
  // close. Must not be called from a pool thread.
  auto wait_thread_pool() -> void { thread_pool_.waitForEmpty(); }

  // Every map must call this: only waitForEmpty() erases the map's bucket in
  // the process-global scheduler, which the worker loop otherwise keeps
  // walking.
  auto shutdown_thread_pool() -> void {
    thread_pool_.runRenderJobs(/*closeQueue=*/true);
    thread_pool_.waitForEmpty();
  }

  [[nodiscard]] auto renderer_observer() const -> mln::RendererObserver* {
    return observer_.get();
  }

  auto begin_render(uint64_t update_generation) noexcept -> void {
    if (observer_ != nullptr) observer_->begin_render(update_generation);
  }

  // Runs on the map's run loop, and tells the attached session, which renders
  // the image. Zero means no still image is pending.
  auto set_still_image_request_generation(uint64_t generation) -> void {
    if (observer_ != nullptr) {
      observer_->still_image_frames().set_request_generation(generation);
    }
    auto notify_session = std::function<void()>{};
    {
      const std::scoped_lock lock(latest_update_mutex_);
      if (std::exchange(still_image_generation_, generation) == generation)
        return;
      notify_session = session_notices_.still_image;
    }
    if (notify_session) notify_session();
  }

  [[nodiscard]] auto still_image_generation() const -> uint64_t {
    const std::scoped_lock lock(latest_update_mutex_);
    return still_image_generation_;
  }

  // Must run before the map that backs the delegate is torn down.
  auto close_renderer_observer() -> void {
    if (observer_ != nullptr) {
      observer_->close();
    }
  }

  [[nodiscard]] auto getThreadPool() const
    -> const mln::TaggedScheduler& override {
    return thread_pool_;
  }

 private:
  // The task holds only a weak share of the map, and skips a map that retired
  // meanwhile. When the task cannot be queued, the update is announced now.
  auto schedule_announcement() noexcept -> void {
    if (std::exchange(announcement_scheduled_, true)) return;
    try {
      const auto map = event_state_->owner.lock();
      if (!map) return;
      map->runtime_state->executor.invoke_notice(
        [owner = event_state_->owner]() noexcept {
          const auto live = owner.lock();
          if (live != nullptr && live->map != nullptr)
            live->frontend->announce();
        }
      );
    } catch (...) {
      announce();
    }
  }

  mln::util::RunLoop& run_loop_;
  std::shared_ptr<mln::core::MapEventState> event_state_;
  FeatureStateStore& feature_state_;
  std::unique_ptr<ForwardingRendererObserver> observer_;
  mln::TaggedScheduler thread_pool_;
  // Guards every field from latest_update_ through still_image_generation_.
  // Render sessions read the published update from their own threads, and
  // reset() can touch held_update_ outside a command when a disposed map
  // retires.
  mutable std::mutex latest_update_mutex_;
  std::shared_ptr<mln::UpdateParameters> latest_update_;
  std::shared_ptr<mln::UpdateParameters> held_update_;
  std::shared_ptr<const FeatureStateSnapshot> latest_feature_state_;
  std::shared_ptr<const FeatureStateSnapshot> held_feature_state_;
  RenderSessionNotices session_notices_;
  uint64_t latest_update_generation_ = 0;
  uint64_t still_image_generation_ = 0;
  // Runtime-worker only.
  bool update_stored_ = false;
  bool announcement_scheduled_ = false;
  uint32_t command_group_depth_ = 0;
};

}  // namespace mln::core

namespace {

using mln::core::HeadlessFrontend;

auto validate_map_options(const mln_map_options* options) -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }

  if (options->size < sizeof(mln_map_options)) {
    mln::core::set_thread_error("mln_map_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_mask_bits =
    static_cast<uint64_t>(MLN_RUNTIME_EVENT_FLAG_ALL);
  if ((options->event_mask & ~known_mask_bits) != 0U) {
    mln::core::set_thread_error(
      "mln_map_options.event_mask contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto extent_status = mln::core::validate_logical_extent(
    options->initial_extent,
    "initial extent dimensions and scale factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }

  switch (options->map_mode) {
    case MLN_MAP_MODE_CONTINUOUS:
    case MLN_MAP_MODE_STATIC:
    case MLN_MAP_MODE_TILE:
      break;
    default:
      mln::core::set_thread_error("map_mode is invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

auto to_native_map_mode(mln_map_mode mode) -> mln::MapMode {
  switch (mode) {
    case MLN_MAP_MODE_STATIC:
      return mln::MapMode::Static;
    case MLN_MAP_MODE_TILE:
      return mln::MapMode::Tile;
    case MLN_MAP_MODE_CONTINUOUS:
      return mln::MapMode::Continuous;
    default:
      assert(false);
      return mln::MapMode::Continuous;
  }
}

auto is_still_map_mode(mln_map_mode mode) -> bool {
  return mode == MLN_MAP_MODE_STATIC || mode == MLN_MAP_MODE_TILE;
}

auto exception_message(std::exception_ptr error) -> std::string {
  if (!error) {
    return {};
  }
  try {
    std::rethrow_exception(error);
  } catch (const std::exception& exception) {
    return exception.what();
  } catch (...) {
    return "unknown native error";
  }
}

auto validate_lat_lng(mln_lat_lng coordinate) -> mln_status;
auto validate_edge_insets(mln_edge_insets padding) -> mln_status;
auto validate_screen_point(mln_screen_point point) -> mln_status;

// Validates the members of a camera, whose size its container versions when it
// is embedded.
auto validate_camera_fields(const mln_camera_options* camera) -> mln_status {
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_CAMERA_OPTION_CENTER) | MLN_CAMERA_OPTION_ZOOM |
    MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
    MLN_CAMERA_OPTION_CENTER_ALTITUDE | MLN_CAMERA_OPTION_PADDING |
    MLN_CAMERA_OPTION_ANCHOR | MLN_CAMERA_OPTION_ROLL | MLN_CAMERA_OPTION_FOV;
  if ((camera->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_camera_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if ((camera->fields & MLN_CAMERA_OPTION_CENTER) != 0U) {
    const auto status = validate_lat_lng(camera->center);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    ((camera->fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0U &&
     !std::isfinite(camera->center_altitude)) ||
    ((camera->fields & MLN_CAMERA_OPTION_ZOOM) != 0U &&
     !std::isfinite(camera->zoom)) ||
    ((camera->fields & MLN_CAMERA_OPTION_BEARING) != 0U &&
     !std::isfinite(camera->bearing)) ||
    ((camera->fields & MLN_CAMERA_OPTION_PITCH) != 0U &&
     !std::isfinite(camera->pitch)) ||
    ((camera->fields & MLN_CAMERA_OPTION_ROLL) != 0U &&
     !std::isfinite(camera->roll)) ||
    ((camera->fields & MLN_CAMERA_OPTION_FOV) != 0U &&
     !std::isfinite(camera->field_of_view))
  ) {
    mln::core::set_thread_error("enabled camera numeric fields must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((camera->fields & MLN_CAMERA_OPTION_PADDING) != 0U) {
    const auto status = validate_edge_insets(camera->padding);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if ((camera->fields & MLN_CAMERA_OPTION_ANCHOR) != 0U) {
    const auto status = validate_screen_point(camera->anchor);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }

  return MLN_STATUS_OK;
}

auto validate_camera_options(const mln_camera_options* camera) -> mln_status {
  if (camera == nullptr) {
    mln::core::set_thread_error("camera must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (camera->size < sizeof(mln_camera_options)) {
    mln::core::set_thread_error("mln_camera_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return validate_camera_fields(camera);
}

using DoubleMilliseconds = std::chrono::duration<double, std::milli>;

auto max_native_duration_ms() -> double {
  return std::chrono::duration_cast<DoubleMilliseconds>(mln::Duration::max())
    .count();
}

auto duration_from_milliseconds(double milliseconds) -> mln::Duration {
  return std::chrono::duration_cast<mln::Duration>(
    DoubleMilliseconds{milliseconds}
  );
}

// The accepted bound is exclusive because mln::Duration::max() has no exact
// double representation: the nearest double converts back to 2^63 ticks, one
// past the largest representable count. The margin holds for a nanosecond
// duration only, so pin the representation.
static_assert(
  std::is_same_v<mln::Duration, std::chrono::nanoseconds>,
  "the accepted duration bound is derived from a nanosecond mln::Duration"
);

auto is_native_duration_ms(double milliseconds) -> bool {
  return std::isfinite(milliseconds) && milliseconds >= 0.0 &&
         milliseconds < max_native_duration_ms();
}

auto validate_animation_options(const mln_animation_options* animation)
  -> mln_status {
  if (animation == nullptr) {
    return MLN_STATUS_OK;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_ANIMATION_OPTION_DURATION) |
    MLN_ANIMATION_OPTION_VELOCITY | MLN_ANIMATION_OPTION_MIN_ZOOM |
    MLN_ANIMATION_OPTION_EASING | MLN_ANIMATION_OPTION_TRANSITION_ID;
  if ((animation->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_animation_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto& end_handler = animation->end_handler;
  if (
    end_handler.callback == nullptr && end_handler.queue == MLN_HANDLE_NULL &&
    end_handler.release_user_data != nullptr
  ) {
    mln::core::set_thread_error(
      "a disabled camera transition handler must not retain user data"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (animation->fields & MLN_ANIMATION_OPTION_DURATION) != 0U &&
    !is_native_duration_ms(animation->duration_ms)
  ) {
    mln::core::set_thread_error(
      "animation duration_ms must fit the native duration range"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (animation->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0U &&
    (!std::isfinite(animation->velocity) || animation->velocity <= 0.0)
  ) {
    mln::core::set_thread_error(
      "animation velocity must be positive and finite"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (animation->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0U &&
    !std::isfinite(animation->min_zoom)
  ) {
    mln::core::set_thread_error("animation min_zoom must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_EASING) != 0U) {
    const auto easing = animation->easing;
    if (
      !std::isfinite(easing.x1) || !std::isfinite(easing.y1) ||
      !std::isfinite(easing.x2) || !std::isfinite(easing.y2) ||
      easing.x1 < 0.0 || easing.x1 > 1.0 || easing.x2 < 0.0 || easing.x2 > 1.0
    ) {
      mln::core::set_thread_error(
        "animation easing x values must be within "
        "[0, 1] and all easing values "
        "must be finite"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_camera_fit_options(const mln_camera_fit_options* options)
  -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_camera_fit_options)) {
    mln::core::set_thread_error("mln_camera_fit_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_CAMERA_FIT_OPTION_PADDING) |
    MLN_CAMERA_FIT_OPTION_BEARING | MLN_CAMERA_FIT_OPTION_PITCH;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_camera_fit_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_CAMERA_FIT_OPTION_PADDING) != 0U) {
    const auto status = validate_edge_insets(options->padding);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    ((options->fields & MLN_CAMERA_FIT_OPTION_BEARING) != 0U &&
     !std::isfinite(options->bearing)) ||
    ((options->fields & MLN_CAMERA_FIT_OPTION_PITCH) != 0U &&
     !std::isfinite(options->pitch))
  ) {
    mln::core::set_thread_error("camera fit bearing and pitch must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_bound_options(const mln_bound_options* options) -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("bound options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_bound_options)) {
    mln::core::set_thread_error("mln_bound_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_BOUND_OPTION_BOUNDS) | MLN_BOUND_OPTION_MIN_ZOOM |
    MLN_BOUND_OPTION_MAX_ZOOM | MLN_BOUND_OPTION_MIN_PITCH |
    MLN_BOUND_OPTION_MAX_PITCH | MLN_BOUND_OPTION_UNBOUNDED;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_bound_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_BOUND_OPTION_BOUNDS) != 0U &&
    (options->fields & MLN_BOUND_OPTION_UNBOUNDED) != 0U
  ) {
    mln::core::set_thread_error(
      "MLN_BOUND_OPTION_BOUNDS and MLN_BOUND_OPTION_UNBOUNDED are mutually "
      "exclusive"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_BOUND_OPTION_BOUNDS) != 0U) {
    const auto status = validate_lat_lng_bounds(options->bounds);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if (
    ((options->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0U &&
     !std::isfinite(options->min_zoom)) ||
    ((options->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0U &&
     !std::isfinite(options->max_zoom)) ||
    ((options->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0U &&
     !std::isfinite(options->min_pitch)) ||
    ((options->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0U &&
     !std::isfinite(options->max_pitch))
  ) {
    mln::core::set_thread_error("bound numeric fields must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0U &&
    (options->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0U &&
    options->min_zoom > options->max_zoom
  ) {
    mln::core::set_thread_error(
      "min_zoom must be less than or equal to max_zoom"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0U &&
    (options->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0U &&
    options->min_pitch > options->max_pitch
  ) {
    mln::core::set_thread_error(
      "min_pitch must be less than or equal to max_pitch"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vec3(mln_vec3 value, const char* name) -> mln_status {
  if (
    !std::isfinite(value.x) || !std::isfinite(value.y) ||
    !std::isfinite(value.z)
  ) {
    mln::core::set_thread_error(name);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_quaternion(mln_quaternion value) -> mln_status {
  if (
    !std::isfinite(value.x) || !std::isfinite(value.y) ||
    !std::isfinite(value.z) || !std::isfinite(value.w)
  ) {
    mln::core::set_thread_error(
      "free camera orientation values must be finite"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (value.x == 0.0 && value.y == 0.0 && value.z == 0.0 && value.w == 0.0) {
    mln::core::set_thread_error(
      "free camera orientation must not be zero length"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_free_camera_options(const mln_free_camera_options* options)
  -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("free camera options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_free_camera_options)) {
    mln::core::set_thread_error("mln_free_camera_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_FREE_CAMERA_OPTION_POSITION) |
    MLN_FREE_CAMERA_OPTION_ORIENTATION;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_free_camera_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0U) {
    const auto status = validate_vec3(
      options->position, "free camera position values must be finite"
    );
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  if ((options->fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0U) {
    const auto status = validate_quaternion(options->orientation);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_debug_options(uint32_t options) -> mln_status {
  constexpr auto known_options =
    static_cast<uint32_t>(MLN_MAP_DEBUG_TILE_BORDERS) |
    MLN_MAP_DEBUG_PARSE_STATUS | MLN_MAP_DEBUG_TIMESTAMPS |
    MLN_MAP_DEBUG_COLLISION | MLN_MAP_DEBUG_OVERDRAW |
    MLN_MAP_DEBUG_STENCIL_CLIP | MLN_MAP_DEBUG_DEPTH_BUFFER;
  if ((options & ~known_options) != 0U) {
    mln::core::set_thread_error("debug options contain unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_frustum_offset(mln_edge_insets offset) -> mln_status {
  if (
    !std::isfinite(offset.top) || !std::isfinite(offset.left) ||
    !std::isfinite(offset.bottom) || !std::isfinite(offset.right)
  ) {
    mln::core::set_thread_error("frustum offset values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    offset.top < 0.0 || offset.left < 0.0 || offset.bottom < 0.0 ||
    offset.right < 0.0
  ) {
    mln::core::set_thread_error(
      "frustum offset values must be greater than or equal to 0"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_viewport_options(const mln_map_viewport_options* options)
  -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("viewport options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_map_viewport_options)) {
    mln::core::set_thread_error("mln_map_viewport_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) |
    MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE |
    MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE |
    MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_map_viewport_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0U) {
    switch (options->north_orientation) {
      case MLN_NORTH_ORIENTATION_UP:
      case MLN_NORTH_ORIENTATION_RIGHT:
      case MLN_NORTH_ORIENTATION_DOWN:
      case MLN_NORTH_ORIENTATION_LEFT:
        break;
      default:
        mln::core::set_thread_error("north_orientation is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0U) {
    switch (options->constrain_mode) {
      case MLN_CONSTRAIN_MODE_NONE:
      case MLN_CONSTRAIN_MODE_HEIGHT_ONLY:
      case MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT:
      case MLN_CONSTRAIN_MODE_SCREEN:
        break;
      default:
        mln::core::set_thread_error("constrain_mode is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0U) {
    switch (options->viewport_mode) {
      case MLN_VIEWPORT_MODE_DEFAULT:
      case MLN_VIEWPORT_MODE_FLIPPED_Y:
        break;
      default:
        mln::core::set_thread_error("viewport_mode is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0U) {
    return validate_frustum_offset(options->frustum_offset);
  }
  return MLN_STATUS_OK;
}

auto validate_tile_options(const mln_map_tile_options* options) -> mln_status {
  if (options == nullptr) {
    mln::core::set_thread_error("tile options must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (options->size < sizeof(mln_map_tile_options)) {
    mln::core::set_thread_error("mln_map_tile_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) |
    MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS | MLN_MAP_TILE_OPTION_LOD_SCALE |
    MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD |
    MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT | MLN_MAP_TILE_OPTION_LOD_MODE;
  if ((options->fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_map_tile_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0U &&
    options->prefetch_zoom_delta > std::numeric_limits<uint8_t>::max()
  ) {
    mln::core::set_thread_error("prefetch_zoom_delta must be at most 255");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0U &&
     !std::isfinite(options->lod_min_radius)) ||
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0U &&
     !std::isfinite(options->lod_scale)) ||
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0U &&
     !std::isfinite(options->lod_pitch_threshold)) ||
    ((options->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0U &&
     !std::isfinite(options->lod_zoom_shift))
  ) {
    mln::core::set_thread_error("tile LOD values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0U) {
    switch (options->lod_mode) {
      case MLN_TILE_LOD_MODE_DEFAULT:
      case MLN_TILE_LOD_MODE_DISTANCE:
        break;
      default:
        mln::core::set_thread_error("lod_mode is invalid");
        return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_lat_lng(mln_lat_lng coordinate) -> mln_status {
  if (
    !std::isfinite(coordinate.latitude) || coordinate.latitude < -90.0 ||
    coordinate.latitude > 90.0 || !std::isfinite(coordinate.longitude)
  ) {
    mln::core::set_thread_error(
      "latitude must be finite and within [-90, 90], and longitude must be "
      "finite"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln_status {
  const auto southwest_status = validate_lat_lng(bounds.southwest);
  if (southwest_status != MLN_STATUS_OK) {
    return southwest_status;
  }
  const auto northeast_status = validate_lat_lng(bounds.northeast);
  if (northeast_status != MLN_STATUS_OK) {
    return northeast_status;
  }
  if (
    bounds.southwest.latitude > bounds.northeast.latitude ||
    bounds.southwest.longitude > bounds.northeast.longitude
  ) {
    mln::core::set_thread_error(
      "bounds southwest must be less than or equal to northeast"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_lat_lng_array(
  const mln_lat_lng* coordinates, size_t coordinate_count, bool allow_empty
) -> mln_status {
  if (coordinate_count == 0) {
    if (allow_empty) {
      return MLN_STATUS_OK;
    }
    mln::core::set_thread_error("coordinate_count must be greater than 0");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (coordinates == nullptr) {
    mln::core::set_thread_error("coordinates must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto coordinate_span =
    std::span<const mln_lat_lng>{coordinates, coordinate_count};
  for (const auto coordinate : coordinate_span) {
    const auto status = validate_lat_lng(coordinate);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_screen_point(mln_screen_point point) -> mln_status {
  if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
    mln::core::set_thread_error("screen point values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_screen_point_array(
  const mln_screen_point* points, size_t point_count
) -> mln_status {
  if (point_count == 0) {
    return MLN_STATUS_OK;
  }

  if (points == nullptr) {
    mln::core::set_thread_error("points must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto point_span =
    std::span<const mln_screen_point>{points, point_count};
  for (const auto point : point_span) {
    const auto status = validate_screen_point(point);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_edge_insets(mln_edge_insets padding) -> mln_status {
  if (
    !std::isfinite(padding.top) || !std::isfinite(padding.left) ||
    !std::isfinite(padding.bottom) || !std::isfinite(padding.right) ||
    padding.top < 0.0 || padding.left < 0.0 || padding.bottom < 0.0 ||
    padding.right < 0.0
  ) {
    mln::core::set_thread_error(
      "padding values must be finite and greater than or equal to 0"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_projected_meters(mln_projected_meters meters) -> mln_status {
  if (!std::isfinite(meters.northing) || !std::isfinite(meters.easting)) {
    mln::core::set_thread_error("projected meter values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto to_native_screen_point(mln_screen_point point) -> mln::ScreenCoordinate;
auto from_native_screen_point(const mln::ScreenCoordinate& point)
  -> mln_screen_point;
auto to_native_edge_insets(mln_edge_insets padding) -> mln::EdgeInsets;
auto from_native_edge_insets(const mln::EdgeInsets& insets) -> mln_edge_insets;

auto to_native_camera(const mln_camera_options& camera) -> mln::CameraOptions {
  auto result = mln::CameraOptions{};
  if ((camera.fields & MLN_CAMERA_OPTION_CENTER) != 0U) {
    result.withCenter(to_native_lat_lng(camera.center));
  }
  if ((camera.fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0U) {
    result.withCenterAltitude(camera.center_altitude);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_PADDING) != 0U) {
    result.withPadding(to_native_edge_insets(camera.padding));
  }
  if ((camera.fields & MLN_CAMERA_OPTION_ANCHOR) != 0U) {
    result.withAnchor(to_native_screen_point(camera.anchor));
  }
  if ((camera.fields & MLN_CAMERA_OPTION_ZOOM) != 0U) {
    result.withZoom(camera.zoom);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_BEARING) != 0U) {
    result.withBearing(camera.bearing);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_PITCH) != 0U) {
    result.withPitch(camera.pitch);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_ROLL) != 0U) {
    result.withRoll(camera.roll);
  }
  if ((camera.fields & MLN_CAMERA_OPTION_FOV) != 0U) {
    result.withFov(camera.field_of_view);
  }
  return result;
}

auto from_native_camera(const mln::CameraOptions& camera)
  -> mln_camera_options {
  auto result = mln::core::camera_options_default();
  if (camera.center) {
    result.fields |= MLN_CAMERA_OPTION_CENTER;
    result.center = from_native_lat_lng(*camera.center);
  }
  if (camera.centerAltitude) {
    result.fields |= MLN_CAMERA_OPTION_CENTER_ALTITUDE;
    result.center_altitude = *camera.centerAltitude;
  }
  if (camera.padding) {
    result.fields |= MLN_CAMERA_OPTION_PADDING;
    result.padding = from_native_edge_insets(*camera.padding);
  }
  if (camera.anchor) {
    result.fields |= MLN_CAMERA_OPTION_ANCHOR;
    result.anchor = from_native_screen_point(*camera.anchor);
  }
  if (camera.zoom) {
    result.fields |= MLN_CAMERA_OPTION_ZOOM;
    result.zoom = *camera.zoom;
  }
  if (camera.bearing) {
    result.fields |= MLN_CAMERA_OPTION_BEARING;
    result.bearing = *camera.bearing;
  }
  if (camera.pitch) {
    result.fields |= MLN_CAMERA_OPTION_PITCH;
    result.pitch = *camera.pitch;
  }
  if (camera.roll) {
    result.fields |= MLN_CAMERA_OPTION_ROLL;
    result.roll = *camera.roll;
  }
  if (camera.fov) {
    result.fields |= MLN_CAMERA_OPTION_FOV;
    result.field_of_view = *camera.fov;
  }
  return result;
}

// Converts camera options that MapLibre Native reports with every member but
// the anchor set, as a transform's current camera always is.
auto from_native_complete_camera(const mln::CameraOptions& camera)
  -> mln_camera {
  return mln_camera{
    .center = from_native_lat_lng(camera.center.value()),
    .center_altitude = camera.centerAltitude.value(),
    .padding = from_native_edge_insets(camera.padding.value()),
    .zoom = camera.zoom.value(),
    .bearing = camera.bearing.value(),
    .pitch = camera.pitch.value(),
    .roll = camera.roll.value(),
    .field_of_view = camera.fov.value(),
  };
}

// The MapLibre transitions that one camera command with an end handler
// started. A command that starts several transitions, such as a camera delta
// that pans and zooms, gives each of them a copy of one finish function, and
// the copies share this state. The command ends once, after the last of its
// transitions finishes, as cancelled when a C API cancellation finished any of
// them. MapLibre invokes the finish functions on the runtime worker,
// immediately before the camera change that ends each transition.
//
// The state holds event state by value, so it stays valid for as long as
// MapLibre keeps a copy. When MapLibre drops the last copy without finishing
// every transition, as when a command fails before it starts them, the
// command ends as cancelled.
struct CommandTransitions {
  std::shared_ptr<mln::core::MapEventState> event_state;
  std::shared_ptr<mln::core::CameraTransitionEnd> end;
  std::optional<uint64_t> transition_id;
  uint32_t remaining = 0;
  bool cancelled = false;

  CommandTransitions(
    std::shared_ptr<mln::core::MapEventState> events,
    std::shared_ptr<mln::core::CameraTransitionEnd> handler,
    std::optional<uint64_t> id, uint32_t transitions
  )
      : event_state(std::move(events)),
        end(std::move(handler)),
        transition_id(id),
        remaining(transitions) {}
  CommandTransitions(const CommandTransitions&) = delete;
  CommandTransitions(CommandTransitions&&) = delete;
  auto operator=(const CommandTransitions&) -> CommandTransitions& = delete;
  auto operator=(CommandTransitions&&) -> CommandTransitions& = delete;

  ~CommandTransitions() {
    if (remaining == 0) return;
    mln::core::end_camera_command(
      *event_state, end, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED
    );
  }

  auto finish() -> void {
    if (remaining == 0) return;
    const auto& cancellation = event_state->cancelling_transitions;
    if (
      cancellation.active &&
      (cancellation.all || transition_id == cancellation.transition_id)
    ) {
      cancelled = true;
    }
    remaining -= 1;
    if (remaining != 0) return;
    mln::core::end_camera_command(
      *event_state, end,
      cancelled ? MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED
                : MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED
    );
  }
};

struct CommandTransitionFinish {
  std::shared_ptr<CommandTransitions> command;

  auto operator()() const -> void { command->finish(); }
};

// Copies the command's end handler, or leaves out a disabled one.
auto camera_transition_end(
  const mln_animation_options& animation,
  std::shared_ptr<mln::core::CameraTransitionEnd>& out,
  std::unique_ptr<mln::delivery::QueuedRegistration>& registration
) -> mln_status {
  const auto& handler = animation.end_handler;
  if (handler.callback == nullptr && handler.queue == MLN_HANDLE_NULL) {
    return MLN_STATUS_OK;
  }
  auto queue = std::shared_ptr<mln::delivery::DeliveryQueueState>{};
  const auto queue_status = mln::delivery::resolve_registration_queue(
    "camera transition handler", handler.queue, handler.callback != nullptr,
    handler.release_user_data != nullptr, queue
  );
  if (queue_status != MLN_STATUS_OK) return queue_status;
  auto copied = handler;
  if (queue != nullptr) {
    registration = std::make_unique<mln::delivery::QueuedRegistration>(
      std::move(queue), handler.user_data
    );
    const auto end = mln_camera_transition_end{};
    registration->reserve<MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END>(
      mln_camera_transition_end_delivery{.end = &end}
    );
    copied.callback = &mln::core::CameraTransitionEnd::queue_end;
    copied.user_data = registration.get();
    copied.release_user_data = &mln::delivery::QueuedRegistration::retire;
    copied.queue = MLN_HANDLE_NULL;
  }
  out = std::make_shared<mln::core::CameraTransitionEnd>(copied);
  return MLN_STATUS_OK;
}

template <typename Submit>
auto submit_with_transition_end(
  const mln_animation_options& animation, Submit submit
) -> mln_status {
  auto end = std::shared_ptr<mln::core::CameraTransitionEnd>{};
  auto registration = std::unique_ptr<mln::delivery::QueuedRegistration>{};
  const auto end_status = camera_transition_end(animation, end, registration);
  if (end_status != MLN_STATUS_OK) return end_status;
  if (end == nullptr) return submit(end);
  end->transfer_to_runtime();
  auto status = MLN_STATUS_NATIVE_ERROR;
  try {
    status = submit(end);
  } catch (...) {
    end->return_to_caller();
    throw;
  }
  if (status == MLN_STATUS_OK) {
    static_cast<void>(registration.release());
  } else {
    end->return_to_caller();
  }
  return status;
}

// MapLibre Native owns the returned AnimationOptions for the transition
// lifetime. `transitions` is the number of MapLibre transitions that the
// command starts with these options; see CommandTransitions. A command with
// an end handler builds its options first, before anything that can fail, so
// that a failure ends the command as cancelled.
auto to_native_animation(
  const std::shared_ptr<mln::core::MapEventState>& event_state,
  const mln_animation_options& animation,
  const std::shared_ptr<mln::core::CameraTransitionEnd>& end,
  uint32_t transitions = 1
) -> mln::AnimationOptions {
  auto result = mln::AnimationOptions{};
  if ((animation.fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0U) {
    result.transitionId = animation.transition_id;
  }
  if (end != nullptr) {
    event_state->running_transition_ends.push_back(end);
    auto command = std::shared_ptr<CommandTransitions>{};
    try {
      command = std::make_shared<CommandTransitions>(
        event_state, end, result.transitionId, transitions
      );
    } catch (...) {
      mln::core::end_camera_command(
        *event_state, end, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED
      );
      throw;
    }
    result.transitionFinishFn = CommandTransitionFinish{std::move(command)};
  }
  if ((animation.fields & MLN_ANIMATION_OPTION_DURATION) != 0U) {
    result.duration = duration_from_milliseconds(animation.duration_ms);
  }
  if ((animation.fields & MLN_ANIMATION_OPTION_VELOCITY) != 0U) {
    result.velocity = animation.velocity;
  }
  if ((animation.fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0U) {
    result.minZoom = animation.min_zoom;
  }
  if ((animation.fields & MLN_ANIMATION_OPTION_EASING) != 0U) {
    const auto easing = animation.easing;
    result.easing.emplace(easing.x1, easing.y1, easing.x2, easing.y2);
  }
  return result;
}

auto camera_fit_padding(const mln_camera_fit_options* options)
  -> mln::EdgeInsets {
  if (
    options == nullptr ||
    (options->fields & MLN_CAMERA_FIT_OPTION_PADDING) == 0U
  ) {
    return mln::EdgeInsets{};
  }
  return to_native_edge_insets(options->padding);
}

auto camera_fit_bearing(const mln_camera_fit_options* options)
  -> std::optional<double> {
  if (
    options == nullptr ||
    (options->fields & MLN_CAMERA_FIT_OPTION_BEARING) == 0U
  ) {
    return std::nullopt;
  }
  return options->bearing;
}

auto camera_fit_pitch(const mln_camera_fit_options* options)
  -> std::optional<double> {
  if (
    options == nullptr || (options->fields & MLN_CAMERA_FIT_OPTION_PITCH) == 0U
  ) {
    return std::nullopt;
  }
  return options->pitch;
}

auto to_native_debug_options(uint32_t options) -> mln::MapDebugOptions {
  return static_cast<mln::MapDebugOptions>(options);
}

auto from_native_debug_options(mln::MapDebugOptions options) -> uint32_t {
  return static_cast<uint32_t>(options);
}

auto to_native_north_orientation(mln_north_orientation orientation)
  -> mln::NorthOrientation {
  switch (orientation) {
    case MLN_NORTH_ORIENTATION_RIGHT:
      return mln::NorthOrientation::Rightwards;
    case MLN_NORTH_ORIENTATION_DOWN:
      return mln::NorthOrientation::Downwards;
    case MLN_NORTH_ORIENTATION_LEFT:
      return mln::NorthOrientation::Leftwards;
    case MLN_NORTH_ORIENTATION_UP:
      return mln::NorthOrientation::Upwards;
    default:
      assert(false);
      return mln::NorthOrientation::Upwards;
  }
}

auto from_native_north_orientation(mln::NorthOrientation orientation)
  -> mln_north_orientation {
  switch (orientation) {
    case mln::NorthOrientation::Rightwards:
      return MLN_NORTH_ORIENTATION_RIGHT;
    case mln::NorthOrientation::Downwards:
      return MLN_NORTH_ORIENTATION_DOWN;
    case mln::NorthOrientation::Leftwards:
      return MLN_NORTH_ORIENTATION_LEFT;
    case mln::NorthOrientation::Upwards:
      return MLN_NORTH_ORIENTATION_UP;
  }
  assert(false);
  return MLN_NORTH_ORIENTATION_UP;
}

auto to_native_constrain_mode(mln_constrain_mode mode) -> mln::ConstrainMode {
  switch (mode) {
    case MLN_CONSTRAIN_MODE_NONE:
      return mln::ConstrainMode::None;
    case MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT:
      return mln::ConstrainMode::WidthAndHeight;
    case MLN_CONSTRAIN_MODE_SCREEN:
      return mln::ConstrainMode::Screen;
    case MLN_CONSTRAIN_MODE_HEIGHT_ONLY:
      return mln::ConstrainMode::HeightOnly;
    default:
      assert(false);
      return mln::ConstrainMode::HeightOnly;
  }
}

auto from_native_constrain_mode(mln::ConstrainMode mode) -> mln_constrain_mode {
  switch (mode) {
    case mln::ConstrainMode::None:
      return MLN_CONSTRAIN_MODE_NONE;
    case mln::ConstrainMode::WidthAndHeight:
      return MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT;
    case mln::ConstrainMode::Screen:
      return MLN_CONSTRAIN_MODE_SCREEN;
    case mln::ConstrainMode::HeightOnly:
      return MLN_CONSTRAIN_MODE_HEIGHT_ONLY;
  }
  assert(false);
  return MLN_CONSTRAIN_MODE_HEIGHT_ONLY;
}

auto to_native_viewport_mode(mln_viewport_mode mode) -> mln::ViewportMode {
  switch (mode) {
    case MLN_VIEWPORT_MODE_FLIPPED_Y:
      return mln::ViewportMode::FlippedY;
    case MLN_VIEWPORT_MODE_DEFAULT:
      return mln::ViewportMode::Default;
    default:
      assert(false);
      return mln::ViewportMode::Default;
  }
}

auto from_native_viewport_mode(mln::ViewportMode mode) -> mln_viewport_mode {
  switch (mode) {
    case mln::ViewportMode::FlippedY:
      return MLN_VIEWPORT_MODE_FLIPPED_Y;
    case mln::ViewportMode::Default:
      return MLN_VIEWPORT_MODE_DEFAULT;
  }
  assert(false);
  return MLN_VIEWPORT_MODE_DEFAULT;
}

auto from_native_edge_insets(const mln::EdgeInsets& insets) -> mln_edge_insets {
  return mln_edge_insets{
    .top = insets.top(),
    .left = insets.left(),
    .bottom = insets.bottom(),
    .right = insets.right()
  };
}

auto to_native_tile_lod_mode(mln_tile_lod_mode mode) -> mln::TileLodMode {
  switch (mode) {
    case MLN_TILE_LOD_MODE_DISTANCE:
      return mln::TileLodMode::Distance;
    case MLN_TILE_LOD_MODE_DEFAULT:
      return mln::TileLodMode::Default;
    default:
      assert(false);
      return mln::TileLodMode::Default;
  }
}

auto from_native_tile_lod_mode(mln::TileLodMode mode) -> mln_tile_lod_mode {
  switch (mode) {
    case mln::TileLodMode::Distance:
      return MLN_TILE_LOD_MODE_DISTANCE;
    case mln::TileLodMode::Default:
      return MLN_TILE_LOD_MODE_DEFAULT;
  }
  assert(false);
  return MLN_TILE_LOD_MODE_DEFAULT;
}

auto to_native_lat_lng(mln_lat_lng coordinate) -> mln::LatLng {
  return mln::LatLng{coordinate.latitude, coordinate.longitude};
}

auto from_native_lat_lng(const mln::LatLng& coordinate) -> mln_lat_lng {
  return mln_lat_lng{
    .latitude = coordinate.latitude(), .longitude = coordinate.longitude()
  };
}

auto to_native_lat_lng_bounds(mln_lat_lng_bounds bounds) -> mln::LatLngBounds {
  return mln::LatLngBounds::hull(
    to_native_lat_lng(bounds.southwest), to_native_lat_lng(bounds.northeast)
  );
}

auto from_native_lat_lng_bounds(const mln::LatLngBounds& bounds)
  -> mln_lat_lng_bounds {
  return mln_lat_lng_bounds{
    .southwest =
      mln_lat_lng{.latitude = bounds.south(), .longitude = bounds.west()},
    .northeast =
      mln_lat_lng{.latitude = bounds.north(), .longitude = bounds.east()}
  };
}

// mbgl keeps the unbounded flag private. Its operator== treats unbounded values
// as equal to each other and distinct from every bounded one, so comparing
// against a default-constructed value tests the flag exactly.
auto is_unbounded_lat_lng_bounds(const mln::LatLngBounds& bounds) -> bool {
  return bounds == mln::LatLngBounds{};
}

auto to_native_lat_lngs(const mln_lat_lng* coordinates, size_t coordinate_count)
  -> std::vector<mln::LatLng> {
  auto result = std::vector<mln::LatLng>{};
  result.reserve(coordinate_count);
  const auto coordinate_span =
    std::span<const mln_lat_lng>{coordinates, coordinate_count};
  for (const auto coordinate : coordinate_span) {
    result.emplace_back(to_native_lat_lng(coordinate));
  }
  return result;
}

auto to_native_screen_point(mln_screen_point point) -> mln::ScreenCoordinate {
  return mln::ScreenCoordinate{point.x, point.y};
}

auto from_native_screen_point(const mln::ScreenCoordinate& point)
  -> mln_screen_point {
  return mln_screen_point{.x = point.x, .y = point.y};
}

auto to_native_edge_insets(mln_edge_insets padding) -> mln::EdgeInsets {
  return mln::EdgeInsets{
    padding.top, padding.left, padding.bottom, padding.right
  };
}

auto to_native_bound_options(const mln_bound_options& options)
  -> mln::BoundOptions {
  auto result = mln::BoundOptions{};
  if ((options.fields & MLN_BOUND_OPTION_BOUNDS) != 0U) {
    result.withLatLngBounds(to_native_lat_lng_bounds(options.bounds));
  }
  if ((options.fields & MLN_BOUND_OPTION_UNBOUNDED) != 0U) {
    // A default-constructed LatLngBounds is the mbgl unbounded constraint.
    result.withLatLngBounds(mln::LatLngBounds{});
  }
  if ((options.fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0U) {
    result.withMinZoom(options.min_zoom);
  }
  if ((options.fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0U) {
    result.withMaxZoom(options.max_zoom);
  }
  if ((options.fields & MLN_BOUND_OPTION_MIN_PITCH) != 0U) {
    result.withMinPitch(options.min_pitch);
  }
  if ((options.fields & MLN_BOUND_OPTION_MAX_PITCH) != 0U) {
    result.withMaxPitch(options.max_pitch);
  }
  return result;
}

auto from_native_bound_options(const mln::BoundOptions& options)
  -> mln_bound_options {
  auto result = mln::core::bound_options_default();
  if (options.bounds) {
    if (is_unbounded_lat_lng_bounds(*options.bounds)) {
      result.fields |= MLN_BOUND_OPTION_UNBOUNDED;
    } else {
      result.fields |= MLN_BOUND_OPTION_BOUNDS;
      result.bounds = from_native_lat_lng_bounds(*options.bounds);
    }
  }
  if (options.minZoom) {
    result.fields |= MLN_BOUND_OPTION_MIN_ZOOM;
    result.min_zoom = *options.minZoom;
  }
  if (options.maxZoom) {
    result.fields |= MLN_BOUND_OPTION_MAX_ZOOM;
    result.max_zoom = *options.maxZoom;
  }
  if (options.minPitch) {
    result.fields |= MLN_BOUND_OPTION_MIN_PITCH;
    result.min_pitch = *options.minPitch;
  }
  if (options.maxPitch) {
    result.fields |= MLN_BOUND_OPTION_MAX_PITCH;
    result.max_pitch = *options.maxPitch;
  }
  return result;
}

auto to_native_vec3(mln_vec3 value) -> mln::vec3 {
  return mln::vec3{{value.x, value.y, value.z}};
}

auto from_native_vec3(const mln::vec3& value) -> mln_vec3 {
  const auto [x_component, y_component, z_component] = value;
  return mln_vec3{.x = x_component, .y = y_component, .z = z_component};
}

auto to_native_vec4(mln_quaternion value) -> mln::vec4 {
  return mln::vec4{{value.x, value.y, value.z, value.w}};
}

auto from_native_vec4(const mln::vec4& value) -> mln_quaternion {
  const auto [x_component, y_component, z_component, w_component] = value;
  return mln_quaternion{
    .x = x_component, .y = y_component, .z = z_component, .w = w_component
  };
}

auto to_native_free_camera(const mln_free_camera_options& options)
  -> mln::FreeCameraOptions {
  auto result = mln::FreeCameraOptions{};
  if ((options.fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0U) {
    result.position = to_native_vec3(options.position);
  }
  if ((options.fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0U) {
    result.orientation = to_native_vec4(options.orientation);
  }
  return result;
}

auto from_native_free_camera(const mln::FreeCameraOptions& options)
  -> mln_free_camera_options {
  auto result = mln::core::free_camera_options_default();
  if (options.position) {
    result.fields |= MLN_FREE_CAMERA_OPTION_POSITION;
    result.position = from_native_vec3(*options.position);
  }
  if (options.orientation) {
    result.fields |= MLN_FREE_CAMERA_OPTION_ORIENTATION;
    result.orientation = from_native_vec4(*options.orientation);
  }
  return result;
}

// A view's camera has finite members, and its zoom, pitch, and field of view
// stay within MapLibre Native's absolute limits, the only range in which a
// transform converts coordinates.
auto validate_map_view(const mln_map_view& view) -> mln_status {
  constexpr auto projection_mode_fields =
    static_cast<uint32_t>(MLN_PROJECTION_MODE_AXONOMETRIC) |
    MLN_PROJECTION_MODE_X_SKEW | MLN_PROJECTION_MODE_Y_SKEW;
  constexpr auto viewport_fields =
    static_cast<uint32_t>(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) |
    MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE |
    MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE |
    MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  const auto& camera = view.camera;
  if (
    const auto status = validate_lat_lng(camera.center); status != MLN_STATUS_OK
  ) {
    return status;
  }
  if (
    !std::isfinite(camera.center_altitude) || !std::isfinite(camera.zoom) ||
    !std::isfinite(camera.bearing) || !std::isfinite(camera.pitch) ||
    !std::isfinite(camera.roll) || !std::isfinite(camera.field_of_view)
  ) {
    mln::core::set_thread_error("view camera numeric fields must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    const auto status = validate_edge_insets(camera.padding);
    status != MLN_STATUS_OK
  ) {
    return status;
  }
  static const auto limits = mln::TransformState{};
  if (
    camera.zoom < mln::util::MIN_ZOOM || camera.zoom > mln::util::MAX_ZOOM ||
    camera.pitch < mln::util::rad2deg(mln::util::PITCH_MIN) ||
    camera.pitch > mln::util::rad2deg(mln::util::PITCH_MAX) ||
    camera.field_of_view < mln::util::rad2deg(limits.getMinFieldOfView()) ||
    camera.field_of_view > mln::util::rad2deg(limits.getMaxFieldOfView())
  ) {
    mln::core::set_thread_error(
      "view camera zoom, pitch, and field of view must be within MapLibre "
      "Native's limits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    const auto status = mln::core::validate_logical_extent(
      view.extent, "view extent dimensions and scale_factor must be positive"
    );
    status != MLN_STATUS_OK
  ) {
    return status;
  }
  const auto& mode = view.projection_mode;
  if (
    mode.fields != projection_mode_fields || !std::isfinite(mode.x_skew) ||
    !std::isfinite(mode.y_skew)
  ) {
    mln::core::set_thread_error(
      "view projection mode must set every field, with finite skews"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((view.viewport.fields & viewport_fields) != viewport_fields) {
    mln::core::set_thread_error("view viewport options must set every field");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto viewport = view.viewport;
  viewport.size = sizeof(mln_map_viewport_options);
  return validate_viewport_options(&viewport);
}

// Assigns a validated view to a fresh transform. A camera jump would wrap the
// center into the first world copy and clamp zoom and pitch to constraints
// that the view does not carry, so the state is set directly, with MapLibre
// Native's widest limits and no constraint until every member is in place.
auto transform_from_map_view(const mln_map_view& view) -> mln::TransformState {
  const auto& camera = view.camera;
  auto state = mln::TransformState{
    mln::ConstrainMode::None,
    to_native_viewport_mode(view.viewport.viewport_mode)
  };
  state.setMaxZoom(mln::util::MAX_ZOOM);
  state.setMaxPitch(mln::util::PITCH_MAX);
  state.setSize(mln::Size{view.extent.width, view.extent.height});
  state.setNorthOrientation(
    to_native_north_orientation(view.viewport.north_orientation)
  );
  state.setFrustumOffset(to_native_edge_insets(view.viewport.frustum_offset));
  state.setEdgeInsets(to_native_edge_insets(camera.padding));
  state.setLatLngZoom(to_native_lat_lng(camera.center), camera.zoom);
  state.setCenterAltitude(camera.center_altitude);
  state.setBearing(-mln::util::deg2rad(camera.bearing));
  state.setPitch(mln::util::deg2rad(camera.pitch));
  state.setRoll(mln::util::deg2rad(camera.roll));
  state.setFieldOfView(mln::util::deg2rad(camera.field_of_view));
  state.setAxonometric(view.projection_mode.axonometric);
  state.setXSkew(view.projection_mode.x_skew);
  state.setYSkew(view.projection_mode.y_skew);
  state.setConstrainMode(
    to_native_constrain_mode(view.viewport.constrain_mode)
  );
  return state;
}

}  // namespace

namespace mln::core {
auto map_view_from_transform(
  const mln::TransformState& transform, double scale_factor
) -> mln_map_view {
  auto camera =
    from_native_complete_camera(transform.getCameraOptions(std::nullopt));
  // The center keeps its world copy, which a later conversion needs and which
  // camera options may wrap.
  camera.center =
    from_native_lat_lng(transform.getLatLng(mln::LatLng::Unwrapped));
  const auto size = transform.getSize();
  return mln_map_view{
    .size = sizeof(mln_map_view),
    .camera = camera,
    .extent =
      {
        .width = size.width, .height = size.height, .scale_factor = scale_factor
      },
    .projection_mode =
      {.size = sizeof(mln_projection_mode),
       .fields = static_cast<uint32_t>(MLN_PROJECTION_MODE_AXONOMETRIC) |
                 MLN_PROJECTION_MODE_X_SKEW | MLN_PROJECTION_MODE_Y_SKEW,
       .axonometric = transform.getAxonometric(),
       .x_skew = transform.getXSkew(),
       .y_skew = transform.getYSkew()},
    .viewport = {
      .size = sizeof(mln_map_viewport_options),
      .fields =
        static_cast<uint32_t>(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) |
        MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE |
        MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE |
        MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET,
      .north_orientation =
        from_native_north_orientation(transform.getNorthOrientation()),
      .constrain_mode =
        from_native_constrain_mode(transform.getConstrainMode()),
      .viewport_mode = from_native_viewport_mode(transform.getViewportMode()),
      .frustum_offset = from_native_edge_insets(transform.getFrustumOffset())
    }
  };
}

auto validate_debug_options_input(uint32_t options) -> mln_status {
  return validate_debug_options(options);
}

auto validate_viewport_options_input(const mln_map_viewport_options* options)
  -> mln_status {
  return validate_viewport_options(options);
}

auto validate_tile_options_input(const mln_map_tile_options* options)
  -> mln_status {
  return validate_tile_options(options);
}

auto validate_bound_options_input(const mln_bound_options* options)
  -> mln_status {
  return validate_bound_options(options);
}

auto validate_free_camera_options_input(const mln_free_camera_options* options)
  -> mln_status {
  return validate_free_camera_options(options);
}

MapObject::~MapObject() = default;

MapProjectionObject::~MapProjectionObject() = default;

namespace {

class RuntimeMapRetainGuard final {
 public:
  explicit RuntimeMapRetainGuard(mln_runtime runtime) noexcept
      : runtime_(runtime) {}

  ~RuntimeMapRetainGuard() { release_runtime_map(runtime_); }

  RuntimeMapRetainGuard(const RuntimeMapRetainGuard&) = delete;
  RuntimeMapRetainGuard(RuntimeMapRetainGuard&&) = delete;
  auto operator=(const RuntimeMapRetainGuard&)
    -> RuntimeMapRetainGuard& = delete;
  auto operator=(RuntimeMapRetainGuard&&) -> RuntimeMapRetainGuard& = delete;

  auto dismiss() noexcept -> void { runtime_ = MLN_HANDLE_NULL; }

 private:
  mln_runtime runtime_ = MLN_HANDLE_NULL;
};

// Runs on the runtime worker from MapLibre's still-image continuation and
// resolves the handle that identifies the pending request.
auto finish_still_image_request(mln_map map, std::exception_ptr error) -> void {
  auto* live = handle_table<MapObject>().try_resolve(map);
  if (live == nullptr) {
    return;
  }
  live->still_image_request_pending = false;
  live->frontend->set_still_image_request_generation(0);
  auto operation = std::exchange(live->still_image_operation, {});
  if (auto release = std::exchange(live->still_image_release_submission, {})) {
    release();
  }
  if (!operation) return;
  operation->complete(
    error ? MLN_STATUS_NATIVE_ERROR : MLN_STATUS_OK,
    error ? exception_message(error) : std::string{}, std::any{std::monostate{}}
  );
}

// The caller holds the map handle table's mutex, so it can act on the result
// without the handle being retired in between.
auto validate_map_live_locked(mln_map map, MapObject*& out_map) -> mln_status {
  out_map = handle_table<MapObject>().resolve_locked(map);
  if (out_map == nullptr) return recorded_handle_fault_status();
  if (out_map->disposal_requested.load()) {
    out_map = nullptr;
    return report_handle_fault(
      HandleTraits<MapObject>::kind, map, HandleFault::Stale
    );
  }
  if (!out_map->created.load(std::memory_order_acquire)) {
    out_map = nullptr;
    set_thread_error("map creation has not succeeded");
    return MLN_STATUS_INVALID_STATE;
  }
  return MLN_STATUS_OK;
}

auto lease_map(mln_map map) -> std::shared_ptr<MapObject> {
  auto live = handle_table<MapObject>().lease(map);
  if (live != nullptr && live->disposal_requested.load()) {
    static_cast<void>(report_handle_fault(
      HandleTraits<MapObject>::kind, map, HandleFault::Stale
    ));
    return nullptr;
  }
  return live;
}

// Runs on the runtime worker. Creation is a map's first submission, so later
// work finds no native map only when creation failed.
auto map_creation_failed(const MapObject& live) -> bool {
  if (live.map != nullptr) return false;
  set_thread_error("map creation failed");
  return true;
}

auto publish_map_snapshot(MapObject& live) -> uint64_t {
  if (!live.snapshot_config_dirty) {
    const auto generation = live.next_snapshot_generation++;
    const auto update_generation = live.frontend->latest_update_generation();
    const auto fully_loaded = live.map->isFullyLoaded();
    const auto gesture = live.map->isGestureInProgress();
    const auto transition = live.map->inTransition();
    const auto refresh_camera = live.snapshot_camera_dirty;
    auto view = mln_map_view{};
    auto free_camera = mln_free_camera_options{};
    if (refresh_camera) {
      view = map_view_from_transform(
        live.map->getTransformState(), live.logical_extent.scale_factor
      );
      free_camera = from_native_free_camera(live.map->getFreeCameraOptions());
    }
    {
      const auto lock = std::scoped_lock{live.snapshot_mutex};
      live.snapshot.generation = generation;
      live.snapshot.event_mask =
        live.event_state->mask.load(std::memory_order_relaxed);
      live.snapshot.fully_loaded = fully_loaded;
      live.snapshot.gesture_in_progress = gesture;
      live.snapshot.transition_in_progress = transition;
      live.snapshot.latest_render_update_generation = update_generation;
      if (refresh_camera) {
        live.snapshot.view = view;
        live.snapshot.free_camera = free_camera;
      }
    }
    live.snapshot_camera_dirty = false;
    live.event_state->published_generation = generation;
    return generation;
  }

  auto snapshot = mln_map_snapshot{
    .size = sizeof(mln_map_snapshot),
    .debug_options = from_native_debug_options(live.map->getDebug()),
    .generation = live.next_snapshot_generation++,
    .fully_loaded = live.map->isFullyLoaded(),
    .rendering_stats_view_enabled = live.map->isRenderingStatsViewEnabled(),
    .gesture_in_progress = live.map->isGestureInProgress(),
    .transition_in_progress = live.map->inTransition(),
    .event_mask = live.event_state->mask.load(std::memory_order_relaxed),
    .latest_render_update_generation =
      live.frontend->latest_update_generation(),
    .tile =
      {.size = sizeof(mln_map_tile_options),
       .fields =
         static_cast<uint32_t>(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) |
         MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS | MLN_MAP_TILE_OPTION_LOD_SCALE |
         MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD |
         MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT | MLN_MAP_TILE_OPTION_LOD_MODE,
       .prefetch_zoom_delta = live.map->getPrefetchZoomDelta(),
       .lod_min_radius = live.map->getTileLodMinRadius(),
       .lod_scale = live.map->getTileLodScale(),
       .lod_pitch_threshold = live.map->getTileLodPitchThreshold(),
       .lod_zoom_shift = live.map->getTileLodZoomShift(),
       .lod_mode = from_native_tile_lod_mode(live.map->getTileLodMode())},
    .bounds = from_native_bound_options(live.map->getBounds()),
    .free_camera = from_native_free_camera(live.map->getFreeCameraOptions()),
    .view = map_view_from_transform(
      live.map->getTransformState(), live.logical_extent.scale_factor
    )
  };
  const auto generation = snapshot.generation;
  {
    const std::scoped_lock lock(live.snapshot_mutex);
    live.snapshot = snapshot;
  }
  live.snapshot_config_dirty = false;
  live.snapshot_camera_dirty = false;
  live.event_state->published_generation = generation;
  return generation;
}

// Runs the handlers of the ended commands from a later task on the runtime
// worker, outside the MapLibre call that ended them. The task first notifies
// the attached session of the update before the handlers run. The map keeps its
// runtime executor running, so the task runs unless allocation fails, in
// which case the handlers run here.
auto post_ended_transitions(MapEventState& events, uint64_t generation) noexcept
  -> void {
  if (events.ended_transitions.empty()) return;
  auto delivery = EndedTransitionDelivery{
    std::exchange(events.ended_transitions, {}), generation
  };
  try {
    const auto owner = events.owner.lock();
    if (owner != nullptr) {
      owner->runtime_state->executor.invoke(
        [delivery = std::move(delivery),
         weak = events.owner]() mutable -> void {
          if (const auto live = weak.lock(); live && live->map)
            live->frontend->announce();
          delivery.deliver();
        }
      );
      return;
    }
  } catch (...) {
  }
}

// Opens a transaction, which holds the events that the map raises until
// close_map_transaction() stamps them with the generation it publishes.
// Commands run as tasks on the runtime worker and never run another task
// inline, so a transaction never opens inside another.
auto open_map_transaction(MapEventState& events) -> void {
  assert(events.transaction_depth == 0 && "map transactions never nest");
  events.transaction_depth += 1;
}

// Queues the held events with `generation` and announces the held update to
// the attached session from a later task. Ended camera transitions run their
// handlers with the transaction's generation. Every delivery is
// attempted, and the first failure is rethrown afterwards.
auto close_map_transaction(MapObject& live, uint64_t generation) -> void {
  auto& events = *live.event_state;
  auto error = std::exception_ptr{};
  const auto record = [&error] {
    if (!error) error = std::current_exception();
  };
  events.transaction_depth -= 1;
  try {
    queue_held_map_events(
      events, generation != 0 ? generation : events.published_generation
    );
  } catch (...) {
    record();
  }
  try {
    live.frontend->release_held_update();
  } catch (...) {
    record();
  }
  deliver_ended_transitions(
    events, generation != 0 ? generation : events.published_generation
  );
  if (error) std::rethrow_exception(error);
}

struct MapCommandOutcome {
  mln_status status = MLN_STATUS_NATIVE_ERROR;
  CommandGenerations generations;
  std::string message;
};

// Runs a map command's work on the runtime worker as one transaction. The map
// publishes a snapshot after the work, whether it committed or failed, and its
// events are queued with that generation before the caller completes the
// command. Work returns a status and sets the thread error for a failure, and
// an exception fails the command with its text. Work can run host callbacks,
// such as the release callbacks of a style load, so the transaction keeps its
// own shares of the map and its event state.
template <typename Work>
auto run_map_transaction(std::shared_ptr<MapObject> map, Work&& work)
  -> MapCommandOutcome {
  auto& live = *map;
  const auto event_state = live.event_state;
  auto outcome = MapCommandOutcome{};
  if (map_creation_failed(live)) {
    outcome.status = MLN_STATUS_INVALID_STATE;
    outcome.message = thread_last_error_message();
    return outcome;
  }
  open_map_transaction(*event_state);
  clear_thread_error();
  try {
    outcome.status = std::invoke(std::forward<Work>(work), live);
    // A diagnostic is the failure's text; completion.h promises the message
    // is empty on success, so a swallowed one is not attached.
    if (outcome.status != MLN_STATUS_OK) {
      outcome.message = thread_last_error_message();
    }
  } catch (...) {
    outcome.status = MLN_STATUS_NATIVE_ERROR;
    outcome.message = exception_message(std::current_exception());
  }
  try {
    // The snapshot reports the update that the work produced, unless a
    // command group still holds it.
    live.frontend->store_held_update();
    outcome.generations = {
      .snapshot = publish_map_snapshot(live),
      .render_update = live.frontend->pending_update_generation(),
    };
  } catch (...) {
    if (outcome.status == MLN_STATUS_OK) {
      outcome.status = MLN_STATUS_NATIVE_ERROR;
      outcome.message = exception_message(std::current_exception());
    }
  }
  try {
    close_map_transaction(live, outcome.generations.snapshot);
  } catch (...) {
    if (outcome.status == MLN_STATUS_OK) {
      outcome.status = MLN_STATUS_NATIVE_ERROR;
      outcome.message = exception_message(std::current_exception());
    }
  }
  return outcome;
}

auto complete_map_command(Completion* completion, MapCommandOutcome outcome)
  -> void {
  complete_command(
    completion,
    outcome.status == MLN_STATUS_OK ? MLN_COMMAND_DISPOSITION_COMMITTED
                                    : MLN_COMMAND_DISPOSITION_FAILED,
    outcome.status, outcome.generations, std::move(outcome.message)
  );
}

}  // namespace

auto MapEventState::publish() -> uint64_t {
  const auto locked = owner.lock();
  if (locked == nullptr || locked->map == nullptr) return 0;
  return publish_map_snapshot(*locked);
}

auto map_options_default() noexcept -> mln_map_options {
  return mln_map_options{
    .size = sizeof(mln_map_options),
    .initial_extent =
      {.width = default_map_width,
       .height = default_map_height,
       .scale_factor = default_scale_factor},
    .map_mode = MLN_MAP_MODE_CONTINUOUS,
    .fast_pfor_enabled = false,
    .event_mask = MLN_RUNTIME_EVENT_FLAG_ALL
  };
}

auto camera_options_default() noexcept -> mln_camera_options {
  return mln_camera_options{
    .size = sizeof(mln_camera_options),
    .fields = 0,
    .center = {.latitude = 0, .longitude = 0},
    .center_altitude = 0,
    .padding = {.top = 0, .left = 0, .bottom = 0, .right = 0},
    .anchor = {.x = 0, .y = 0},
    .zoom = 0,
    .bearing = 0,
    .pitch = 0,
    .roll = 0,
    .field_of_view = 0
  };
}

auto animation_options_default() noexcept -> mln_animation_options {
  return mln_animation_options{
    .fields = 0,
    .duration_ms = 0,
    .velocity = 0,
    .min_zoom = 0,
    .easing = {.x1 = 0, .y1 = 0, .x2 = 0.25, .y2 = 1},
    .transition_id = 0,
    .end_handler = {
      .callback = nullptr,
      .user_data = nullptr,
      .release_user_data = nullptr,
      .queue = MLN_HANDLE_NULL
    }
  };
}

auto camera_delta_default() noexcept -> mln_camera_delta {
  return mln_camera_delta{
    .size = sizeof(mln_camera_delta),
    .fields = 0,
    .offset = {},
    .scale = 0,
    .bearing = 0,
    .pitch = 0,
    .anchor = {},
    .animation = animation_options_default(),
    .gesture_phase = MLN_GESTURE_PHASE_NONE,
    .reserved = 0
  };
}

auto camera_update_default() noexcept -> mln_camera_update {
  return mln_camera_update{
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = camera_options_default(),
    .animation = animation_options_default(),
    .gesture_phase = MLN_GESTURE_PHASE_NONE,
    .reserved = 0
  };
}

auto camera_fit_options_default() noexcept -> mln_camera_fit_options {
  return mln_camera_fit_options{
    .size = sizeof(mln_camera_fit_options),
    .fields = 0,
    .padding = {.top = 0, .left = 0, .bottom = 0, .right = 0},
    .bearing = 0,
    .pitch = 0
  };
}

auto bound_options_default() noexcept -> mln_bound_options {
  return mln_bound_options{
    .size = sizeof(mln_bound_options),
    .fields = 0,
    .bounds =
      {.southwest = {.latitude = 0, .longitude = 0},
       .northeast = {.latitude = 0, .longitude = 0}},
    .min_zoom = 0,
    .max_zoom = 0,
    .min_pitch = 0,
    .max_pitch = 0
  };
}

auto free_camera_options_default() noexcept -> mln_free_camera_options {
  return mln_free_camera_options{
    .size = sizeof(mln_free_camera_options),
    .fields = 0,
    .position = {.x = 0, .y = 0, .z = 0},
    .orientation = {.x = 0, .y = 0, .z = 0, .w = 1}
  };
}

auto projection_mode_default() noexcept -> mln_projection_mode {
  return mln_projection_mode{
    .size = sizeof(mln_projection_mode),
    .fields = 0,
    .axonometric = false,
    .x_skew = 0,
    .y_skew = 0
  };
}

auto map_viewport_options_default() noexcept -> mln_map_viewport_options {
  return mln_map_viewport_options{
    .size = sizeof(mln_map_viewport_options),
    .fields = 0,
    .north_orientation = MLN_NORTH_ORIENTATION_UP,
    .constrain_mode = MLN_CONSTRAIN_MODE_HEIGHT_ONLY,
    .viewport_mode = MLN_VIEWPORT_MODE_DEFAULT,
    .frustum_offset = {.top = 0, .left = 0, .bottom = 0, .right = 0}
  };
}

auto map_tile_options_default() noexcept -> mln_map_tile_options {
  return mln_map_tile_options{
    .size = sizeof(mln_map_tile_options),
    .fields = 0,
    .prefetch_zoom_delta = 0,
    .lod_min_radius = 0,
    .lod_scale = 0,
    .lod_pitch_threshold = 0,
    .lod_zoom_shift = 0,
    .lod_mode = MLN_TILE_LOD_MODE_DEFAULT
  };
}

auto validate_map_live(mln_map map, MapObject*& out_map) -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  return validate_map_live_locked(map, out_map);
}

namespace {

struct MapSubmissionContext {
  std::shared_ptr<MapObject> map;
  std::shared_ptr<ControlLease> control;
  std::shared_ptr<RuntimeObject> runtime;
};

auto acquire_map_submission(mln_map map, MapSubmissionContext& out_context)
  -> mln_status {
  auto live = lease_map(map);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!live->control.acquire()) {
    set_thread_error("map is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  auto guard = ControlLease{&live->control};
  auto control = std::make_shared<ControlLease>(std::move(guard));
  auto runtime = live->runtime_state;
  if (runtime->disposal_requested.load()) {
    return report_handle_fault(
      HandleTraits<RuntimeObject>::kind, live->runtime, HandleFault::Stale
    );
  }
  out_context = MapSubmissionContext{
    .map = std::move(live),
    .control = std::move(control),
    .runtime = std::move(runtime),
  };
  return MLN_STATUS_OK;
}

}  // namespace

auto execute_map_command(MapCommandTask& task) -> void {
  complete_map_command(
    task.completion, run_map_transaction(task.map, [&task](MapObject&) {
      return task.map_work(task);
    })
  );
}

auto prepare_map_command_task(
  mln_map map, MapCommandTask& task, const mln_completion* descriptor
) -> mln_status {
  if (descriptor) {
    const auto status = make_completion(descriptor, task.embedded_completion);
    if (status != MLN_STATUS_OK) return status;
    task.completion = &*task.embedded_completion;
  }
  task.map = lease_map(map);
  if (!task.map) return recorded_handle_fault_status();
  if (!task.map->control.acquire()) {
    set_thread_error("map is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  task.map_lease = ControlLease{&task.map->control};
  auto runtime = task.map->runtime_state;
  if (runtime->disposal_requested.load()) {
    return report_handle_fault(
      HandleTraits<RuntimeObject>::kind, task.map->runtime, HandleFault::Stale
    );
  }
  return MLN_STATUS_OK;
}

auto submit_map_command_task(
  mln_map map, MapCommandTask& task, const mln_completion* descriptor
) -> mln_status {
  const auto status = prepare_map_command_task(map, task, descriptor);
  if (status != MLN_STATUS_OK) return status;
  const auto runtime = task.map->runtime_state;
  return submit_runtime_command_task(runtime, task);
}

auto start_style_operation(
  mln_map map, StyleDelivery delivery, StyleWork work,
  const mln_completion* completion
) -> mln_status {
  auto completion_state = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, completion_state);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  auto result_callback = [completion_state, delivery](
                           mln_status status, std::string diagnostic,
                           std::any result
                         ) mutable {
    auto shared = std::any_cast<std::shared_ptr<StyleOperationResult>>(&result);
    if (status != MLN_STATUS_OK || shared == nullptr || *shared == nullptr) {
      complete_failure(
        completion_state,
        status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
        status == MLN_STATUS_OK ? "style operation produced an invalid result"
                                : std::move(diagnostic)
      );
      return;
    }
    completion_state->resolve([delivery, result = std::move(*shared)](
                                const CompletionSink& descriptor
                              ) { delivery(descriptor, *result); });
  };
  auto context = MapSubmissionContext{};
  const auto acquire_status = acquire_map_submission(map, context);
  if (acquire_status != MLN_STATUS_OK) return acquire_status;
  auto state = std::make_shared<OperationObject>(std::move(result_callback));
  const auto submission = submit_runtime_operation(
    context.runtime, state,
    [live = std::move(context.map), control = std::move(context.control), state,
     work = std::move(work)]() mutable -> void {
      // The control lease is captured so map teardown waits for this work.
      static_cast<void>(control);
      auto result = std::make_shared<StyleOperationResult>();
      clear_thread_error();
      if (map_creation_failed(*live)) {
        state->complete(
          MLN_STATUS_INVALID_STATE, thread_last_error_message(),
          std::any{result}
        );
        return;
      }
      try {
        const auto status = std::invoke(std::move(work), *live, *result);
        state->complete(status, thread_last_error_message(), std::any{result});
      } catch (const std::exception& exception) {
        state->complete(MLN_STATUS_NATIVE_ERROR, exception.what(), result);
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, "style operation failed", result
        );
      }
    },
    completion_state
  );
  return submission;
}

namespace {

template <auto Function>
using GeometryValue = std::conditional_t<
  CompletionValue<Function>::array,
  std::vector<typename CompletionValue<Function>::Type>,
  typename CompletionValue<Function>::Type>;

template <auto Function>
using GeometryWork =
  std::function<mln_status(MapObject&, GeometryValue<Function>&)>;

template <auto Function>
auto start_geometry_operation(
  mln_map map, GeometryWork<Function> work, const mln_completion* completion
) -> mln_status {
  auto completion_state = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, completion_state);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  auto context = MapSubmissionContext{};
  const auto acquire_status = acquire_map_submission(map, context);
  if (acquire_status != MLN_STATUS_OK) return acquire_status;
  auto state =
    std::make_shared<OperationObject>([completion_state](
                                        mln_status status,
                                        std::string diagnostic, std::any result
                                      ) mutable {
      auto* value = std::any_cast<GeometryValue<Function>>(&result);
      if (status != MLN_STATUS_OK || value == nullptr) {
        complete_failure(
          completion_state,
          status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
          status == MLN_STATUS_OK
            ? "geometry operation produced an invalid result"
            : std::move(diagnostic)
        );
        return;
      }
      completion_state->resolve(
        [value = std::move(*value)](const CompletionSink& descriptor) {
          CompletionValue<Function>::deliver(descriptor, value);
        }
      );
    });
  const auto submission = submit_runtime_operation(
    context.runtime, state,
    [live = std::move(context.map), control = std::move(context.control), state,
     work = std::move(work)]() mutable {
      // The control lease is captured so map teardown waits for this work.
      static_cast<void>(control);
      auto result = GeometryValue<Function>{};
      clear_thread_error();
      if (map_creation_failed(*live)) {
        state->complete(
          MLN_STATUS_INVALID_STATE, thread_last_error_message(),
          std::any{std::move(result)}
        );
        return;
      }
      try {
        const auto status = std::invoke(std::move(work), *live, result);
        state->complete(
          status, thread_last_error_message(), std::any{std::move(result)}
        );
      } catch (const std::exception& exception) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception.what(), std::any{std::move(result)}
        );
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, "geometry operation failed",
          std::any{std::move(result)}
        );
      }
    },
    completion_state
  );
  return submission;
}

}  // namespace

namespace {

// Runs work against a live projection on the calling thread, serialized with
// every other projection call, including close, by the per-projection mutex.
template <typename Work>
auto with_projection(mln_map_projection projection, Work work) -> mln_status {
  auto live = handle_table<MapProjectionObject>().lease(projection);
  if (live == nullptr) return recorded_handle_fault_status();
  mln::testing::hit(mln::testing::SyncPoint::ProjectionCallLeased);
  const std::scoped_lock call_lock(live->call_mutex);
  if (live->projection == nullptr) {
    return report_handle_fault(
      HandleTraits<MapProjectionObject>::kind, projection, HandleFault::Stale
    );
  }
  mln::testing::hit(mln::testing::SyncPoint::ProjectionCallRunning);
  work(*live->projection);
  return MLN_STATUS_OK;
}

}  // namespace

namespace {

// Runs on the runtime worker as the map's first submission, so every later
// submission finds the native map built or, if building it failed, absent.
auto construct_map(MapObject& live, const mln_map_options& effective) -> void {
  // Construction is a transaction, so the events that MapLibre raises from
  // its constructor carry the first published generation.
  open_map_transaction(*live.event_state);
  try {
    mln::testing::hit(mln::testing::SyncPoint::MapConstruction);
    auto map_options = mln::MapOptions{};
    map_options.withMapMode(to_native_map_mode(effective.map_mode))
      .withSize(
        mln::Size{
          effective.initial_extent.width, effective.initial_extent.height
        }
      )
      .withPixelRatio(static_cast<float>(effective.initial_extent.scale_factor))
      .withFastPFOREnabled(effective.fast_pfor_enabled);
    live.map = std::make_unique<mln::Map>(
      *live.frontend, *live.observer, map_options,
      resource_options_for_runtime(*live.runtime_state)
    );
    live.callback_sources->attach(*live.map);
    live.frontend->store_held_update();
    close_map_transaction(live, publish_map_snapshot(live));
  } catch (...) {
    live.callback_sources->detach();
    live.map.reset();
    if (live.event_state->in_transaction()) {
      try {
        close_map_transaction(live, 0);
      } catch (...) {
      }
    }
    throw;
  }
  live.created.store(true, std::memory_order_release);
}

}  // namespace

auto create_map_start(
  mln_runtime runtime, const mln_map_options* options, mln_map* out_map,
  const mln_completion* completion
) -> mln_status {
  const auto options_status = validate_map_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  if (out_map == nullptr) {
    set_thread_error("out_map must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_map != MLN_HANDLE_NULL) {
    set_thread_error("out_map must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto runtime_state = lease_runtime(runtime);
  if (runtime_state == nullptr) return recorded_handle_fault_status();
  const auto effective = options == nullptr ? map_options_default() : *options;
  auto completion_state = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, completion_state);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  const auto retain_status = retain_runtime_map(runtime);
  if (retain_status != MLN_STATUS_OK) {
    return retain_status;
  }
  auto retain_guard = RuntimeMapRetainGuard{runtime};

  auto owned_map = std::make_shared<MapObject>();
  owned_map->runtime = runtime;
  owned_map->runtime_state = runtime_state;
  owned_map->map_mode = effective.map_mode;
  owned_map->logical_extent = effective.initial_extent;
  owned_map->event_state = std::make_shared<MapEventState>();
  owned_map->callback_sources = std::make_shared<CallbackSourceRegistry>();
  owned_map->event_state->mask.store(
    effective.event_mask, std::memory_order_relaxed
  );
  owned_map->event_state->runtime_events = runtime_state->event_state;
  owned_map->event_state->owner = owned_map;
  owned_map->observer = std::make_unique<HeadlessObserver>(
    owned_map->event_state, owned_map->callback_sources
  );
  owned_map->frontend = std::make_unique<HeadlessFrontend>(
    runtime_run_loop(runtime_state.get()), owned_map->event_state,
    owned_map->feature_state
  );
  auto state = std::make_shared<OperationObject>([completion_state](
                                                   mln_status status,
                                                   std::string diagnostic,
                                                   std::any /*result*/
                                                 ) mutable {
    if (status != MLN_STATUS_OK) {
      complete_failure(completion_state, status, std::move(diagnostic));
      return;
    }
    valueless_completion<&mln_runtime_create_map>().complete(completion_state);
  });
  if (!runtime_state->control.acquire()) {
    set_thread_error("runtime is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  owned_map->runtime_cleanup_lease = ControlLease{&runtime_state->control};
  // A new map cannot be closing, so this acquire succeeds.
  static_cast<void>(owned_map->control.acquire());
  auto creation = std::make_shared<ControlLease>(&owned_map->control);
  auto work = std::function<void()>{
    [live = owned_map, creation = std::move(creation), state,
     effective]() mutable -> void {
      // The creation lease is captured so map release waits for this work.
      static_cast<void>(creation);
      try {
        construct_map(*live, effective);
        state->complete(MLN_STATUS_OK, {}, std::any{std::monostate{}});
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    }
  };
  // Every allocation above happens before the handle is published, so a throw
  // cannot leave a registered map that no caller holds. The handle resolves
  // before the worker builds the map, so the observer and frontend capture an
  // id that already resolves.
  const auto handle = handle_table<MapObject>().insert(owned_map);
  owned_map->self = handle;
  owned_map->event_state->map = handle;
  // The completion may run before this call returns, so the caller's handle
  // is written before the work that runs it is submitted.
  *out_map = handle;
  const auto submit_status = submit_runtime_operation(
    runtime_state, state, std::move(work), completion_state
  );
  if (submit_status != MLN_STATUS_OK) {
    *out_map = MLN_HANDLE_NULL;
    static_cast<void>(handle_table<MapObject>().remove(handle));
    return submit_status;
  }
  retain_guard.dismiss();
  mln::testing::hit(mln::testing::SyncPoint::MapCreationSubmitted);
  return MLN_STATUS_OK;
}

auto map_snapshot_get(mln_map map, mln_map_snapshot* out_snapshot)
  -> mln_status {
  if (
    out_snapshot == nullptr || out_snapshot->size < sizeof(mln_map_snapshot)
  ) {
    set_thread_error(
      "out_snapshot must not be null and must have a valid size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto live = lease_map(map);
  if (live == nullptr) return recorded_handle_fault_status();
  if (live->control.is_closing()) {
    set_thread_error("map is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  if (!live->created.load(std::memory_order_acquire)) {
    set_thread_error("map creation has not succeeded");
    return MLN_STATUS_INVALID_STATE;
  }
  const std::scoped_lock lock(live->snapshot_mutex);
  *out_snapshot = live->snapshot;
  return MLN_STATUS_OK;
}

namespace {

auto submit_map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion,
  bool from_session
) -> mln_status {
  struct ResizeTask final : MapCommandTask {
    mln_logical_extent extent;
    bool from_session;
    ResizeTask(mln_logical_extent extent, bool from_session)
        : extent(extent), from_session(from_session) {
      command = true;
      run = [](ExecutorTask* base) noexcept {
        auto owner =
          std::unique_ptr<ResizeTask>{static_cast<ResizeTask*>(base)};
        run_runtime_command_task(*owner);
      };
      work = [](RuntimeCommandTask& base) {
        auto& self = static_cast<ResizeTask&>(base);
        auto& live = *self.map;
        if (!self.from_session) {
          auto attached = false;
          {
            const auto lock =
              std::scoped_lock{handle_table<MapObject>().mutex()};
            attached = live.render_target_session != nullptr;
          }
          if (attached) {
            complete_command(
              self.completion, MLN_COMMAND_DISPOSITION_FAILED,
              MLN_STATUS_INVALID_STATE, {},
              "map still has an attached render session"
            );
            return;
          }
        }
        complete_map_command(
          self.completion,
          run_map_transaction(
            self.map, [extent = self.extent](MapObject& live) -> mln_status {
              // Map::setSize publishes an update even when the size is
              // unchanged.
              if (
                live.logical_extent.width == extent.width &&
                live.logical_extent.height == extent.height
              )
                return MLN_STATUS_OK;
              live.logical_extent.width = extent.width;
              live.logical_extent.height = extent.height;
              live.snapshot_camera_dirty = true;
              live.map->setSize(mln::Size{extent.width, extent.height});
              return MLN_STATUS_OK;
            }
          )
        );
      };
      clear_work = [](RuntimeCommandTask& base) noexcept {
        auto& self = static_cast<ResizeTask&>(base);
        self.map_lease.reset();
        self.map.reset();
      };
    }
  };
  auto task = std::make_unique<ResizeTask>(extent, from_session);
  const auto prepared = prepare_map_command_task(map, *task, completion);
  if (prepared != MLN_STATUS_OK) return prepared;
  const auto extent_status = validate_logical_extent(
    extent, "extent dimensions and scale factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) return extent_status;
  if (extent.scale_factor != task->map->logical_extent.scale_factor) {
    set_thread_error("scale factor is fixed at map creation");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto runtime = task->map->runtime_state;
  const auto status = submit_runtime_command_task(runtime, *task);
  if (status == MLN_STATUS_OK) static_cast<void>(task.release());
  return status;
}

}  // namespace

auto map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion
) -> mln_status {
  return submit_map_resize(map, extent, completion, false);
}

namespace {
// Pool shutdown can block, so maps shut down on a separate lane.
auto map_teardown_lane() -> RetirementLane& {
  static auto* lane = new RetirementLane{};
  return *lane;
}

auto finish_map_retirement(RetirementTask* task) noexcept -> void {
  auto& map = *static_cast<MapObject*>(task->context);
  static_cast<void>(handle_table<MapObject>().remove(map.self));
  map.callback_sources->detach();
  map.frontend->close_renderer_observer();
  // A submission leased before close can start a transition after cancellation.
  close_camera_transitions(*map.event_state);
  map.map.reset();
  map.callback_sources->release_all();
  if (auto completion = std::move(map.release_completion)) {
    valueless_completion<&mln_map_release>().complete(completion);
  }
  task->run = [](RetirementTask* node) noexcept {
    auto* retiring = static_cast<MapObject*>(node->context);
    auto owned = std::move(retiring->disposal_owner);
    mln::testing::hit(mln::testing::SyncPoint::MapPoolShutdown);
    owned->frontend->shutdown_thread_pool();
  };
  map_teardown_lane().submit(*task);
}

auto begin_map_retirement(RetirementTask* task) noexcept -> void {
  auto& map = *static_cast<MapObject*>(task->context);
  if (map.still_image_operation != nullptr) {
    map.frontend->set_still_image_request_generation(0);
    map.still_image_operation->complete(MLN_STATUS_CANCELLED, {}, {});
  }
  close_camera_transitions(*map.event_state);
  if (auto release = std::exchange(map.still_image_release_submission, {}))
    release();
  task->run = finish_map_retirement;
  map.control.notify_when_drained(
    [](void* context) noexcept {
      auto& live = *static_cast<MapObject*>(context);
      live.runtime_state->executor.invoke([task = &live.disposal_task] {
        task->run(task);
      });
    },
    &map
  );
}

auto retire_map(
  std::shared_ptr<MapObject> owned, std::shared_ptr<Completion> completion
) noexcept -> void {
  owned->release_completion = std::move(completion);
  owned->disposal_owner = owned;
  owned->disposal_task.context = owned.get();
  owned->disposal_task.run = begin_map_retirement;
  release_runtime_map(owned->runtime);
  auto& executor = owned->runtime_state->executor;
  auto* task = &owned->disposal_task;
  // Drop the caller's reference before the worker can hand ownership to the
  // pool shutdown lane.
  owned.reset();
  executor.invoke([task] { task->run(task); });
}
}  // namespace

auto dispose_map(mln_map map) -> mln_status {
  auto owned = handle_table<MapObject>().lease(map);
  if (owned == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{handle_table<MapObject>().mutex()};
    if (handle_table<MapObject>().resolve_locked(map) != owned.get())
      return recorded_handle_fault_status();
    const auto status = owned->control.begin_close(true);
    if (status != MLN_STATUS_OK) return status;
    owned->disposal_requested = true;
  }
  retire_map(std::move(owned), nullptr);
  return MLN_STATUS_OK;
}

auto release_map(mln_map map, const mln_completion* completion) -> mln_status {
  auto release_completion = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, release_completion);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  auto owned = handle_table<MapObject>().lease(map);
  if (owned == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{handle_table<MapObject>().mutex()};
    if (handle_table<MapObject>().resolve_locked(map) != owned.get()) {
      return recorded_handle_fault_status();
    }
    // Render sessions are the map's only children. Once the host disposed the
    // attached one, only its own retirement can detach it, so the map's
    // retirement waits for that. A child can outlive its cleared pointer
    // while a detach finishes, which the map waits for too.
    if (
      owned->render_target_session != nullptr &&
      !owned->render_target_session_disposed
    ) {
      set_thread_error("map still has an attached render session");
      return MLN_STATUS_INVALID_STATE;
    }
    const auto close_status = owned->control.begin_close(true);
    if (close_status != MLN_STATUS_OK) {
      return close_status;
    }
    owned->disposal_requested = true;
  }
  release_completion->accept();
  retire_map(std::move(owned), std::move(release_completion));
  return MLN_STATUS_OK;
}
auto map_request_repaint(mln_map map, const mln_completion* completion)
  -> mln_status {
  const auto live = lease_map(map);
  if (!live) return recorded_handle_fault_status();
  if (live->map_mode != MLN_MAP_MODE_CONTINUOUS) {
    set_thread_error("map is not in continuous mode");
    return MLN_STATUS_INVALID_STATE;
  }
  return submit_map_command(
    map,
    [](MapObject& live) -> mln_status {
      live.map->triggerRepaint();
      return MLN_STATUS_OK;
    },
    completion
  );
}

auto map_begin_command_group(mln_map map, const mln_completion* completion)
  -> mln_status {
  return submit_map_command(
    map,
    [](MapObject& live) -> mln_status {
      live.frontend->begin_command_group();
      return MLN_STATUS_OK;
    },
    completion
  );
}

auto map_end_command_group(mln_map map, const mln_completion* completion)
  -> mln_status {
  return submit_map_command(
    map,
    [](MapObject& live) -> mln_status {
      if (!live.frontend->end_command_group()) {
        set_thread_error("map has no open command group");
        return MLN_STATUS_INVALID_STATE;
      }
      // The command's own transaction stores the held update when this was
      // the outermost group.
      return MLN_STATUS_OK;
    },
    completion
  );
}

auto map_request_still_image_start(
  mln_map map, const mln_completion* completion
) -> mln_status {
  auto completion_state = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, completion_state);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  auto context = MapSubmissionContext{};
  const auto acquire_status = acquire_map_submission(map, context);
  if (acquire_status != MLN_STATUS_OK) {
    return acquire_status;
  }
  if (!is_still_map_mode(context.map->map_mode)) {
    set_thread_error("map is not in static or tile mode");
    return MLN_STATUS_INVALID_STATE;
  }
  auto live = std::move(context.map);
  auto submission = std::move(context.control);
  auto submission_released = std::make_shared<std::atomic_bool>(false);
  auto release_submission = [submission,
                             submission_released]() noexcept -> void {
    if (!submission_released->exchange(true)) {
      submission->reset();
    }
  };
  auto state = std::make_shared<OperationObject>(
    [completion_state](mln_status status, std::string diagnostic, std::any) {
      valueless_completion<&mln_map_request_still_image>().complete(
        completion_state, status, std::move(diagnostic)
      );
    }
  );
  const auto submit_status = submit_runtime_operation(
    context.runtime, state,
    [map, live = std::move(live), state, release_submission]() mutable -> void {
      if (live->control.is_closing()) {
        state->complete(MLN_STATUS_CANCELLED, {}, {});
        release_submission();
        return;
      }
      if (map_creation_failed(*live)) {
        state->complete(
          MLN_STATUS_INVALID_STATE, thread_last_error_message(), {}
        );
        release_submission();
        return;
      }
      if (live->still_image_request_pending) {
        state->complete(
          MLN_STATUS_INVALID_STATE,
          "map already has a pending still-image request", {}
        );
        return;
      }
      // A held update is not published, so the request would bind to the
      // generation of an update that predates it.
      if (live->frontend->command_group_open()) {
        state->complete(
          MLN_STATUS_INVALID_STATE, "map has an open command group", {}
        );
        return;
      }
      try {
        live->still_image_request_pending = true;
        live->still_image_operation = state;
        live->still_image_release_submission = release_submission;
        live->map->renderStill(
          [map, release_submission](std::exception_ptr error) mutable -> void {
            finish_still_image_request(map, error);
            release_submission();
          }
        );
        // renderStill() published the request's update before returning,
        // unless the request already finished with an error.
        if (live->still_image_request_pending) {
          live->frontend->set_still_image_request_generation(
            live->frontend->latest_update_generation()
          );
        }
      } catch (...) {
        live->still_image_request_pending = false;
        live->still_image_operation.reset();
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    },
    completion_state
  );
  return submit_status;
}

auto map_scale_factor(mln_map map) -> double {
  const auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? default_scale_factor
                         : live->logical_extent.scale_factor;
}

auto map_native(MapObject& map) -> mln::Map& { return *map.map; }

// A render session sizes its map through the same ordered extent command as
// public resize, so one path owns the logical extent after map creation. The
// map keeps the scale factor it was created with, which may differ from the
// session's.
auto map_post_resize(
  mln_map map, uint32_t width, uint32_t height, const mln_completion* completion
) -> mln_status {
  return submit_map_resize(
    map,
    mln_logical_extent{
      .width = width, .height = height, .scale_factor = map_scale_factor(map)
    },
    completion, true
  );
}

auto map_post_trigger_repaint(mln_map map) -> mln_status {
  return map_request_repaint(map, nullptr);
}

auto map_set_render_session_notices(mln_map map, RenderSessionNotices notices)
  -> mln_status {
  const auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) return recorded_handle_fault_status();
  live->frontend->set_session_notices(std::move(notices));
  return MLN_STATUS_OK;
}

auto map_latest_update(MapObject& map)
  -> std::shared_ptr<mln::UpdateParameters> {
  return map.frontend->latest_update();
}

auto map_latest_update_generation(MapObject& map) noexcept -> uint64_t {
  return map.frontend->latest_update_generation();
}

auto map_latest_update_snapshot(MapObject& map) -> RenderUpdateSnapshot {
  return map.frontend->latest_update_snapshot();
}

auto map_still_image_generation(MapObject& map) -> uint64_t {
  return map.frontend->still_image_generation();
}

auto map_begin_render(MapObject& map, uint64_t update_generation) noexcept
  -> void {
  map.frontend->begin_render(update_generation);
}

auto map_renderer_observer(MapObject& map) -> mln::RendererObserver* {
  return map.frontend->renderer_observer();
}

auto map_run_render_jobs(MapObject& map) -> void {
  map.frontend->run_render_jobs();
}

auto map_quiesce_render_workers(mln_map map) -> void {
  // The lease holds the object across the blocking wait, so a concurrent
  // release cannot destroy the frontend under it.
  const auto live = handle_table<MapObject>().try_lease(map);
  if (live != nullptr) {
    live->frontend->wait_thread_pool();
  }
}

// The handle-table lock keeps the map live across the attached-session claim.
auto map_attach_render_target_session(
  mln_map map, void* session, MapObject*& out_map
) -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  MapObject* live = nullptr;
  const auto status = validate_map_live_locked(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (session == nullptr) {
    set_thread_error("render session must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // begin_close() runs under this same lock, so this check makes the claim
  // atomic with close's empty-session preflight.
  if (live->control.is_closing()) {
    set_thread_error("map is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->render_target_session != nullptr) {
    set_thread_error("map already has an attached render session");
    return MLN_STATUS_INVALID_STATE;
  }
  if (!live->control.retain_child()) return MLN_STATUS_INVALID_STATE;
  live->render_target_session = session;
  out_map = live;
  return MLN_STATUS_OK;
}

// The handle-table lock keeps the map live across the attached-session clear.
auto map_detach_render_target_session(mln_map map, void* session)
  -> mln_status {
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{handle_table<MapObject>().mutex()};
    if (session == nullptr) {
      set_thread_error("render session must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (live->render_target_session != session) {
      set_thread_error("render session is not attached to this map");
      return MLN_STATUS_INVALID_STATE;
    }
    live->render_target_session = nullptr;
    live->render_target_session_disposed = false;
  }
  live->control.release_child();
  return MLN_STATUS_OK;
}

auto map_dispose_render_target_session(mln_map map, void* session) noexcept
  -> void {
  const auto live = handle_table<MapObject>().try_lease(map);
  if (live == nullptr) return;
  const auto lock = std::scoped_lock{handle_table<MapObject>().mutex()};
  if (live->render_target_session == session && session != nullptr)
    live->render_target_session_disposed = true;
}

auto map_set_style_url(MapObject& live, const char* url) -> mln_status {
  if (url == nullptr) {
    set_thread_error("url must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // Parse failures reported on this stack use runtime-worker state, so no
  // additional lock or queued diagnostic is needed.
  live.event_state->style_load_failed = false;
  live.event_state->style_load_failure.clear();
  live.map->getStyle().loadURL(url);
  if (live.event_state->style_load_failed) {
    set_thread_error(live.event_state->style_load_failure.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto map_set_style_json(MapObject& live, mln_buffer_view json) -> mln_status {
  if (!validate_bytes(json, "style JSON")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  try {
    live.event_state->style_load_failed = false;
    live.event_state->style_load_failure.clear();
    live.map->getStyle().loadJSON(
      std::string{reinterpret_cast<const char*>(json.data), json.size}
    );
  } catch (const std::exception& exception) {
    // The diagnostic is this call's own status text, so it is set whatever the
    // mask selects; only the event is gated.
    set_thread_error(exception.what());
    auto& events = *live.event_state;
    if (event_selected(events.mask, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED)) {
      queue_map_event(
        events, events.fresh_generation(), MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
        MLN_RUNTIME_EVENT_PAYLOAD_NONE, zeroed_event_payload(), 0,
        exception.what()
      );
    }
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (live.event_state->style_load_failed) {
    set_thread_error(live.event_state->style_load_failure.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

template <auto Function>
auto start_map_string_operation(
  mln_map map, const mln_completion* completion,
  std::function<std::string(MapObject&)> read
) -> mln_status {
  auto completion_state = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, completion_state);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  auto live = lease_map(map);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!live->control.acquire()) {
    set_thread_error("map is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  auto guard = ControlLease{&live->control};
  auto submission = std::make_shared<ControlLease>(std::move(guard));
  auto state =
    std::make_shared<OperationObject>([completion_state](
                                        mln_status status,
                                        std::string diagnostic, std::any result
                                      ) {
      if (status != MLN_STATUS_OK) {
        complete_failure(completion_state, status, std::move(diagnostic));
        return;
      }
      auto* text = std::any_cast<std::string>(&result);
      if (text == nullptr) {
        complete_failure(
          completion_state, MLN_STATUS_NATIVE_ERROR,
          "map string operation produced an invalid result"
        );
        return;
      }
      completion_state->resolve(
        [text = std::move(*text)](const CompletionSink& descriptor) {
          CompletionValue<Function>::deliver(
            descriptor, {.data = text.data(), .size = text.size()}
          );
        }
      );
    });
  const auto runtime = live->runtime_state;
  const auto submit_status = submit_runtime_operation(
    runtime, state,
    [live = std::move(live), state, read = std::move(read),
     submission = std::move(submission)]() mutable -> void {
      if (map_creation_failed(*live)) {
        state->complete(
          MLN_STATUS_INVALID_STATE, thread_last_error_message(), {}
        );
        return;
      }
      try {
        state->complete(MLN_STATUS_OK, {}, std::any{read(*live)});
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    },
    completion_state
  );
  return submit_status;
}

namespace {
auto feature_state_string_from_view(mln_buffer_view value) -> std::string {
  return value.data == nullptr
           ? std::string{}
           : std::string{static_cast<const char*>(value.data), value.size};
}
}  // namespace

auto map_set_feature_state(
  MapObject& live, const mln_feature_state_selector* selector,
  mln::FeatureState state
) -> mln_status {
  if (
    live.feature_state.set(
      feature_state_string_from_view(selector->source_id),
      feature_state_source_layer(*selector),
      feature_state_string_from_view(selector->feature_id), std::move(state)
    )
  ) {
    live.map->triggerRepaint();
  }
  return MLN_STATUS_OK;
}

auto map_get_feature_state_start(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion
) -> mln_status {
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }
  auto source_id = feature_state_string_from_view(selector->source_id);
  auto source_layer = feature_state_source_layer(*selector);
  auto feature_id = feature_state_string_from_view(selector->feature_id);
  return start_map_string_operation<&mln_map_get_feature_state>(
    map, completion,
    [source_id = std::move(source_id), source_layer = std::move(source_layer),
     feature_id = std::move(feature_id)](MapObject& live) -> std::string {
      auto state = live.feature_state.get(source_id, source_layer, feature_id);
      return serialize_json_value(mln::Value{std::move(state)});
    }
  );
}

auto map_remove_feature_state(
  MapObject& live, const mln_feature_state_selector* selector
) -> mln_status {
  if (
    live.feature_state.remove(
      feature_state_string_from_view(selector->source_id),
      feature_state_source_layer(*selector),
      optional_selector_string(
        *selector, MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, selector->feature_id
      ),
      optional_text(selector->state_key)
    )
  ) {
    live.map->triggerRepaint();
  }
  return MLN_STATUS_OK;
}

auto map_loaded_style_json_start(mln_map map, const mln_completion* completion)
  -> mln_status {
  return start_map_string_operation<&mln_map_get_loaded_style_json>(
    map, completion, [](MapObject& live) -> std::string {
      return live.map->getStyle().getJSON();
    }
  );
}

auto map_style_url_start(mln_map map, const mln_completion* completion)
  -> mln_status {
  return start_map_string_operation<&mln_map_get_style_url>(
    map, completion,
    [](MapObject& live) -> std::string { return live.map->getStyle().getURL(); }
  );
}

auto map_set_event_mask(
  mln_map map, uint64_t mask, const mln_completion* completion
) -> mln_status {
  if ((mask & ~static_cast<uint64_t>(MLN_RUNTIME_EVENT_FLAG_ALL)) != 0U) {
    set_thread_error("mask must be valid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return submit_map_command(
    map,
    [mask](MapObject& live) -> mln_status {
      live.event_state->mask.store(mask, std::memory_order_relaxed);
      return MLN_STATUS_OK;
    },
    completion
  );
}

namespace {

template <typename Mutation>
auto submit_camera_command(
  mln_map map, Mutation mutation, const mln_completion* completion
) -> mln_status {
  return submit_map_command(
    map,
    [mutation = std::move(mutation)](MapObject& live) mutable -> mln_status {
      live.snapshot_camera_dirty = true;
      mutation(live);
      return MLN_STATUS_OK;
    },
    completion
  );
}

// BEGIN and UPDATE mark the gesture before the camera write, so MapLibre
// applies its gesture rules to that write.
auto apply_gesture_phase_before(MapObject& live, mln_gesture_phase phase)
  -> void {
  if (phase == MLN_GESTURE_PHASE_BEGIN || phase == MLN_GESTURE_PHASE_UPDATE) {
    live.map->setGestureInProgress(true);
  }
}

auto apply_gesture_phase_after(MapObject& live, mln_gesture_phase phase)
  -> void {
  if (phase == MLN_GESTURE_PHASE_CANCEL) {
    const auto cancellation = TransitionCancellationScope{
      *live.event_state, {.active = true, .all = true}
    };
    live.map->cancelTransitions();
  }
  if (phase == MLN_GESTURE_PHASE_END || phase == MLN_GESTURE_PHASE_CANCEL) {
    live.map->setGestureInProgress(false);
  }
}

}  // namespace

auto map_update_camera(
  mln_map map, const mln_camera_update* update, const mln_completion* completion
) -> mln_status {
  if (update == nullptr || update->size < sizeof(mln_camera_update)) {
    set_thread_error("camera update must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    update->mode > MLN_CAMERA_UPDATE_MODE_FLY ||
    update->gesture_phase > MLN_GESTURE_PHASE_CANCEL
  ) {
    set_thread_error("camera update mode or gesture phase is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto camera_status = validate_camera_fields(&update->camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  const auto animation_status = validate_animation_options(&update->animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }
  const auto copied = *update;
  return submit_with_transition_end(
    copied.animation, [&](const auto& end) -> mln_status {
      return submit_camera_command(
        map,
        [copied, end](MapObject& live) -> void {
          const auto animation =
            to_native_animation(live.event_state, copied.animation, end);
          apply_gesture_phase_before(live, copied.gesture_phase);
          switch (copied.mode) {
            case MLN_CAMERA_UPDATE_MODE_JUMP:
              // MapLibre gives a jump no animation options, so the jump ends
              // its command here, as soon as it applies.
              live.map->jumpTo(to_native_camera(copied.camera));
              if (animation.transitionFinishFn) animation.transitionFinishFn();
              break;
            case MLN_CAMERA_UPDATE_MODE_EASE:
              live.map->easeTo(to_native_camera(copied.camera), animation);
              break;
            case MLN_CAMERA_UPDATE_MODE_FLY:
              live.map->flyTo(to_native_camera(copied.camera), animation);
              break;
            default:
              break;
          }
          apply_gesture_phase_after(live, copied.gesture_phase);
        },
        completion
      );
    }
  );
}

namespace {

constexpr auto camera_delta_anchored_fields =
  static_cast<uint32_t>(MLN_CAMERA_DELTA_SCALE) | MLN_CAMERA_DELTA_BEARING |
  MLN_CAMERA_DELTA_PITCH;

auto validate_camera_delta(const mln_camera_delta* delta) -> mln_status {
  if (delta == nullptr || delta->size < sizeof(mln_camera_delta)) {
    set_thread_error("camera delta must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields = static_cast<uint32_t>(MLN_CAMERA_DELTA_OFFSET) |
                                camera_delta_anchored_fields |
                                MLN_CAMERA_DELTA_ANCHOR;
  const auto fields = delta->fields;
  if ((fields & ~known_fields) != 0U) {
    set_thread_error("mln_camera_delta.fields contains unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (delta->gesture_phase > MLN_GESTURE_PHASE_CANCEL) {
    set_thread_error("camera delta gesture phase is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (fields & MLN_CAMERA_DELTA_OFFSET) != 0U &&
    validate_screen_point(delta->offset) != MLN_STATUS_OK
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (fields & MLN_CAMERA_DELTA_SCALE) != 0U &&
    (!std::isfinite(delta->scale) || delta->scale <= 0)
  ) {
    set_thread_error("camera delta scale must be finite and positive");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    ((fields & MLN_CAMERA_DELTA_BEARING) != 0U &&
     !std::isfinite(delta->bearing)) ||
    ((fields & MLN_CAMERA_DELTA_PITCH) != 0U && !std::isfinite(delta->pitch))
  ) {
    set_thread_error("camera delta bearing and pitch must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((fields & MLN_CAMERA_DELTA_ANCHOR) != 0U) {
    if ((fields & camera_delta_anchored_fields) == 0U) {
      set_thread_error(
        "a camera delta anchor requires scale, bearing, or pitch"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (validate_screen_point(delta->anchor) != MLN_STATUS_OK) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  const auto& animation = delta->animation;
  const auto animation_status = validate_animation_options(&animation);
  if (animation_status != MLN_STATUS_OK) return animation_status;
  // An anchored ease couples center with its fields, so an animated one would
  // replace the animated pan. An immediate pan has already applied when the
  // ease reads the camera.
  constexpr auto anchored_pan =
    static_cast<uint32_t>(MLN_CAMERA_DELTA_OFFSET) | MLN_CAMERA_DELTA_ANCHOR;
  if (
    (fields & anchored_pan) == anchored_pan &&
    (animation.fields & MLN_ANIMATION_OPTION_DURATION) != 0U &&
    animation.duration_ms > 0
  ) {
    set_thread_error(
      "a camera delta anchor combines with offset only in an immediate delta"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

// The camera that a delta's scale, bearing, and pitch reach from the camera as
// it stands. Map::scaleBy() adds log2(scale) to the zoom the same way.
auto camera_delta_target(const mln::Map& map, const mln_camera_delta& delta)
  -> mln::CameraOptions {
  const auto current = map.getCameraOptions();
  auto camera = mln::CameraOptions{};
  if ((delta.fields & MLN_CAMERA_DELTA_SCALE) != 0U) {
    camera.withZoom(current.zoom.value_or(0) + std::log2(delta.scale));
  }
  if ((delta.fields & MLN_CAMERA_DELTA_BEARING) != 0U) {
    camera.withBearing(current.bearing.value_or(0) + delta.bearing);
  }
  if ((delta.fields & MLN_CAMERA_DELTA_PITCH) != 0U) {
    camera.withPitch(current.pitch.value_or(0) + delta.pitch);
  }
  if ((delta.fields & MLN_CAMERA_DELTA_ANCHOR) != 0U) {
    camera.withAnchor(to_native_screen_point(delta.anchor));
  }
  return camera;
}

}  // namespace

auto map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion
) -> mln_status {
  const auto delta_status = validate_camera_delta(delta);
  if (delta_status != MLN_STATUS_OK) return delta_status;
  const auto copied = *delta;
  return submit_with_transition_end(
    copied.animation, [&](const auto& end) -> mln_status {
      return submit_camera_command(
        map,
        [copied, end](MapObject& live) -> void {
          // The pan and the scale, bearing, and pitch change are separate
          // MapLibre transitions, because an explicit center overrides an
          // anchor. The ease reads the camera after the pan, so an immediate
          // pan moves the camera that the anchor refers to. The transaction
          // around this work publishes both in one snapshot and announces one
          // render update for them.
          const auto pans = (copied.fields & MLN_CAMERA_DELTA_OFFSET) != 0U;
          const auto eases =
            (copied.fields & camera_delta_anchored_fields) != 0U;
          const auto transitions =
            static_cast<uint32_t>(pans) + static_cast<uint32_t>(eases);
          const auto animation = to_native_animation(
            live.event_state, copied.animation, end, std::max(transitions, 1U)
          );
          apply_gesture_phase_before(live, copied.gesture_phase);
          if (pans) {
            live.map->moveBy(to_native_screen_point(copied.offset), animation);
          }
          if (eases) {
            live.map->easeTo(camera_delta_target(*live.map, copied), animation);
          }
          // A delta without components starts no transition, so it ends its
          // command here, as an empty camera update does.
          if (transitions == 0 && animation.transitionFinishFn) {
            animation.transitionFinishFn();
          }
          apply_gesture_phase_after(live, copied.gesture_phase);
        },
        completion
      );
    }
  );
}

auto map_cancel_transitions(mln_map map, const mln_completion* completion)
  -> mln_status {
  return submit_camera_command(
    map,
    [](MapObject& live) -> void {
      const auto cancellation = TransitionCancellationScope{
        *live.event_state, {.active = true, .all = true}
      };
      live.map->cancelTransitions();
    },
    completion
  );
}

auto map_cancel_camera_transition(
  mln_map map, uint64_t transition_id, const mln_completion* completion
) -> mln_status {
  return submit_camera_command(
    map,
    [transition_id](MapObject& live) -> void {
      const auto cancellation = TransitionCancellationScope{
        *live.event_state,
        {.active = true, .all = false, .transition_id = transition_id}
      };
      live.map->cancelTransitions(transition_id);
    },
    completion
  );
}

auto map_camera_query_start(mln_map map, const mln_completion* completion)
  -> mln_status {
  auto completion_state = std::shared_ptr<Completion>{};
  if (
    const auto made = make_completion(completion, completion_state);
    made != MLN_STATUS_OK
  ) {
    return made;
  }
  auto live = lease_map(map);
  if (live == nullptr) return recorded_handle_fault_status();
  if (!live->control.acquire()) {
    set_thread_error("map is closing");
    return MLN_STATUS_INVALID_STATE;
  }
  auto guard = ControlLease{&live->control};
  auto submission = std::make_shared<ControlLease>(std::move(guard));
  auto state =
    std::make_shared<OperationObject>([completion_state](
                                        mln_status status,
                                        std::string diagnostic, std::any result
                                      ) {
      auto* value = std::any_cast<mln_camera_query_result>(&result);
      if (status != MLN_STATUS_OK || value == nullptr) {
        complete_failure(
          completion_state,
          status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
          status == MLN_STATUS_OK ? "camera query produced an invalid result"
                                  : std::move(diagnostic)
        );
        return;
      }
      CompletionValue<&mln_map_get_camera>::complete(completion_state, *value);
    });
  const auto runtime = live->runtime_state;
  const auto submit_status = submit_runtime_operation(
    runtime, state,
    [live = std::move(live), state,
     submission = std::move(submission)]() mutable -> void {
      if (map_creation_failed(*live)) {
        state->complete(
          MLN_STATUS_INVALID_STATE, thread_last_error_message(), {}
        );
        return;
      }
      try {
        const auto generation = publish_map_snapshot(*live);
        state->complete(
          MLN_STATUS_OK, {},
          std::any{mln_camera_query_result{
            .generation = generation,
            .camera = from_native_complete_camera(live->map->getCameraOptions())
          }}
        );
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    },
    completion_state
  );
  return submit_status;
}

auto map_set_debug_options(MapObject& live, uint32_t options) -> mln_status {
  const auto options_status = validate_debug_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  live.snapshot_config_dirty = true;
  live.map->setDebug(to_native_debug_options(options));
  return MLN_STATUS_OK;
}

auto map_set_rendering_stats_view_enabled(MapObject& live, bool enabled)
  -> mln_status {
  live.snapshot_config_dirty = true;
  live.map->enableRenderingStatsView(enabled);
  return MLN_STATUS_OK;
}

auto map_dump_debug_logs(MapObject& live) -> mln_status {
  live.map->dumpDebugLogs();
  return MLN_STATUS_OK;
}

auto map_set_viewport_options(
  MapObject& live, const mln_map_viewport_options* options
) -> mln_status {
  const auto options_status = validate_viewport_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  live.snapshot_config_dirty = true;
  live.snapshot_camera_dirty = true;
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0U) {
    live.map->setNorthOrientation(
      to_native_north_orientation(options->north_orientation)
    );
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0U) {
    live.map->setConstrainMode(
      to_native_constrain_mode(options->constrain_mode)
    );
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0U) {
    live.map->setViewportMode(to_native_viewport_mode(options->viewport_mode));
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0U) {
    live.map->setFrustumOffset(to_native_edge_insets(options->frustum_offset));
  }
  return MLN_STATUS_OK;
}

auto map_set_tile_options(MapObject& live, const mln_map_tile_options* options)
  -> mln_status {
  const auto options_status = validate_tile_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  live.snapshot_config_dirty = true;
  if ((options->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0U) {
    live.map->setPrefetchZoomDelta(
      static_cast<uint8_t>(options->prefetch_zoom_delta)
    );
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0U) {
    live.map->setTileLodMinRadius(options->lod_min_radius);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0U) {
    live.map->setTileLodScale(options->lod_scale);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0U) {
    live.map->setTileLodPitchThreshold(options->lod_pitch_threshold);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0U) {
    live.map->setTileLodZoomShift(options->lod_zoom_shift);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0U) {
    live.map->setTileLodMode(to_native_tile_lod_mode(options->lod_mode));
  }
  return MLN_STATUS_OK;
}

auto map_projection_create(
  const mln_map_view* view, mln_map_projection* out_projection
) -> mln_status {
  if (view == nullptr || view->size < sizeof(mln_map_view)) {
    set_thread_error("view must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_projection == nullptr) {
    set_thread_error("out_projection must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_projection != MLN_HANDLE_NULL) {
    set_thread_error("out_projection must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (const auto status = validate_map_view(*view); status != MLN_STATUS_OK) {
    return status;
  }
  auto owned_projection = std::make_shared<MapProjectionObject>();
  owned_projection->projection =
    std::make_unique<mln::MapProjection>(transform_from_map_view(*view));
  *out_projection =
    handle_table<MapProjectionObject>().insert(std::move(owned_projection));
  return MLN_STATUS_OK;
}

auto map_projection_release(mln_map_projection projection) -> void {
  const auto owned = handle_table<MapProjectionObject>().remove(projection);
  if (owned == nullptr) return;
  // Waits for projection calls already running on other threads, then
  // destroys the projection. A racing call that leased the handle before the
  // removal observes the null projection and reports a stale handle.
  auto call_lock = std::unique_lock(owned->call_mutex, std::try_to_lock);
  if (!call_lock.owns_lock()) {
    mln::testing::hit(mln::testing::SyncPoint::ProjectionCloseWaits);
    call_lock.lock();
  }
  owned->projection.reset();
}

auto map_projection_get_camera(
  mln_map_projection projection, mln_camera_options* out_camera
) -> mln_status {
  if (out_camera == nullptr || out_camera->size < sizeof(mln_camera_options)) {
    set_thread_error("out_camera must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return with_projection(projection, [out_camera](mln::MapProjection& value) {
    *out_camera = from_native_camera(value.getCamera());
  });
}

auto map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera
) -> mln_status {
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  const auto native_camera = to_native_camera(*camera);
  return with_projection(
    projection, [&native_camera](mln::MapProjection& value) {
      value.setCamera(native_camera);
    }
  );
}

auto map_projection_pixel_for_lat_lng(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_screen_point* out_point
) -> mln_status {
  if (out_point == nullptr) {
    set_thread_error("out_point must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinate_status = validate_lat_lng(coordinate);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }
  const auto native_coordinate = to_native_lat_lng(coordinate);
  return with_projection(
    projection, [&native_coordinate, out_point](mln::MapProjection& value) {
      *out_point =
        from_native_screen_point(value.pixelForLatLng(native_coordinate));
    }
  );
}

namespace {

auto projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate, mln::LatLng::WrapMode wrap_mode
) -> mln_status {
  if (out_coordinate == nullptr) {
    set_thread_error("out_coordinate must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto point_status = validate_screen_point(point);
  if (point_status != MLN_STATUS_OK) {
    return point_status;
  }
  const auto native_point = to_native_screen_point(point);
  return with_projection(
    projection,
    [&native_point, out_coordinate, wrap_mode](mln::MapProjection& value) {
      *out_coordinate =
        from_native_lat_lng(value.latLngForPixel(native_point, wrap_mode));
    }
  );
}

}  // namespace

auto map_projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate
) -> mln_status {
  return projection_lat_lng_for_pixel(
    projection, point, out_coordinate, mln::LatLng::Wrapped
  );
}

auto map_projection_lat_lng_for_pixel_unwrapped(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate
) -> mln_status {
  return projection_lat_lng_for_pixel(
    projection, point, out_coordinate, mln::LatLng::Unwrapped
  );
}

// A batch converts into scratch storage under one projection lease and copies
// the results out afterwards, so a conversion that throws partway leaves the
// caller's output unchanged.
auto map_projection_pixels_for_lat_lngs(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_screen_point* out_points
) -> mln_status {
  if (coordinate_count != 0 && out_points == nullptr) {
    set_thread_error(
      "out_points must not be null when coordinate_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto status =
    validate_lat_lng_array(coordinates, coordinate_count, true);
  if (status != MLN_STATUS_OK) return status;
  const auto inputs = std::span{coordinates, coordinate_count};
  auto converted = std::vector<mln_screen_point>(coordinate_count);
  return with_projection(projection, [&](mln::MapProjection& value) {
    std::ranges::transform(
      inputs, converted.begin(), [&value](mln_lat_lng coordinate) {
        return from_native_screen_point(
          value.pixelForLatLng(to_native_lat_lng(coordinate))
        );
      }
    );
    std::ranges::copy(converted, out_points);
  });
}

namespace {

auto projection_lat_lngs_for_pixels(
  mln_map_projection projection, const mln_screen_point* points,
  size_t point_count, mln_lat_lng* out_coordinates,
  mln::LatLng::WrapMode wrap_mode
) -> mln_status {
  if (point_count != 0 && out_coordinates == nullptr) {
    set_thread_error(
      "out_coordinates must not be null when point_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto status = validate_screen_point_array(points, point_count);
  if (status != MLN_STATUS_OK) return status;
  const auto inputs = std::span{points, point_count};
  auto converted = std::vector<mln_lat_lng>(point_count);
  return with_projection(projection, [&](mln::MapProjection& value) {
    std::ranges::transform(
      inputs, converted.begin(), [&value, wrap_mode](mln_screen_point point) {
        return from_native_lat_lng(
          value.latLngForPixel(to_native_screen_point(point), wrap_mode)
        );
      }
    );
    std::ranges::copy(converted, out_coordinates);
  });
}

}  // namespace

auto map_projection_lat_lngs_for_pixels(
  mln_map_projection projection, const mln_screen_point* points,
  size_t point_count, mln_lat_lng* out_coordinates
) -> mln_status {
  return projection_lat_lngs_for_pixels(
    projection, points, point_count, out_coordinates, mln::LatLng::Wrapped
  );
}

auto map_projection_lat_lngs_for_pixels_unwrapped(
  mln_map_projection projection, const mln_screen_point* points,
  size_t point_count, mln_lat_lng* out_coordinates
) -> mln_status {
  return projection_lat_lngs_for_pixels(
    projection, points, point_count, out_coordinates, mln::LatLng::Unwrapped
  );
}

namespace {

auto validate_latitude(double latitude) -> mln_status {
  if (!std::isfinite(latitude) || latitude < -90.0 || latitude > 90.0) {
    set_thread_error("latitude must be finite and within [-90, 90]");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace

auto map_projection_meters_per_pixel_at_latitude(
  mln_map_projection projection, double latitude, double* out_meters_per_pixel
) -> mln_status {
  if (out_meters_per_pixel == nullptr) {
    set_thread_error("out_meters_per_pixel must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto status = validate_latitude(latitude);
  if (status != MLN_STATUS_OK) return status;
  return with_projection(
    projection, [latitude, out_meters_per_pixel](mln::MapProjection& live) {
      *out_meters_per_pixel = mln::Projection::getMetersPerPixelAtLatitude(
        latitude, live.getCamera().zoom.value_or(0.0)
      );
    }
  );
}

auto projected_meters_for_lat_lng(
  mln_lat_lng coordinate, mln_projected_meters* out_meters
) -> mln_status {
  if (out_meters == nullptr) {
    set_thread_error("out_meters must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinate_status = validate_lat_lng(coordinate);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }

  const auto meters =
    mln::Projection::projectedMetersForLatLng(to_native_lat_lng(coordinate));
  *out_meters = mln_projected_meters{
    .northing = meters.northing(), .easting = meters.easting()
  };
  return MLN_STATUS_OK;
}

auto lat_lng_for_projected_meters(
  mln_projected_meters meters, mln_lat_lng* out_coordinate
) -> mln_status {
  if (out_coordinate == nullptr) {
    set_thread_error("out_coordinate must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto meters_status = validate_projected_meters(meters);
  if (meters_status != MLN_STATUS_OK) {
    return meters_status;
  }

  *out_coordinate = from_native_lat_lng(
    mln::Projection::latLngForProjectedMeters(
      mln::ProjectedMeters{meters.northing, meters.easting}
    )
  );
  return MLN_STATUS_OK;
}

auto validate_camera_output(mln_camera_options* out_camera) -> mln_status {
  if (out_camera == nullptr || out_camera->size < sizeof(mln_camera_options)) {
    set_thread_error("out_camera must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto map_camera_for_lat_lng_bounds(
  MapObject& live, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  const auto bounds_status = validate_lat_lng_bounds(bounds);
  if (bounds_status != MLN_STATUS_OK) {
    return bounds_status;
  }
  const auto fit_status = validate_camera_fit_options(fit_options);
  if (fit_status != MLN_STATUS_OK) {
    return fit_status;
  }
  const auto output_status = validate_camera_output(out_camera);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  *out_camera = from_native_camera(live.map->cameraForLatLngBounds(
    to_native_lat_lng_bounds(bounds), camera_fit_padding(fit_options),
    camera_fit_bearing(fit_options), camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_camera_for_lat_lngs(
  MapObject& live, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  const auto coordinates_status =
    validate_lat_lng_array(coordinates, coordinate_count, false);
  if (coordinates_status != MLN_STATUS_OK) {
    return coordinates_status;
  }
  const auto fit_status = validate_camera_fit_options(fit_options);
  if (fit_status != MLN_STATUS_OK) {
    return fit_status;
  }
  const auto output_status = validate_camera_output(out_camera);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  *out_camera = from_native_camera(live.map->cameraForLatLngs(
    to_native_lat_lngs(coordinates, coordinate_count),
    camera_fit_padding(fit_options), camera_fit_bearing(fit_options),
    camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_camera_for_geometry(
  MapObject& live, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  auto native_geometry = to_native_geometry(geometry);
  if (!native_geometry) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (geometry_lat_lngs(*native_geometry).empty()) {
    set_thread_error("geometry must contain at least one coordinate");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto fit_status = validate_camera_fit_options(fit_options);
  if (fit_status != MLN_STATUS_OK) {
    return fit_status;
  }
  const auto output_status = validate_camera_output(out_camera);
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  *out_camera = from_native_camera(live.map->cameraForGeometry(
    *native_geometry, camera_fit_padding(fit_options),
    camera_fit_bearing(fit_options), camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_lat_lng_bounds_for_camera(
  MapObject& live, const mln_camera_options* camera,
  mln_lat_lng_bounds* out_bounds
) -> mln_status {
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  if (out_bounds == nullptr) {
    set_thread_error("out_bounds must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_bounds = from_native_lat_lng_bounds(
    live.map->latLngBoundsForCamera(to_native_camera(*camera))
  );
  return MLN_STATUS_OK;
}

auto map_lat_lng_bounds_for_camera_unwrapped(
  MapObject& live, const mln_camera_options* camera,
  mln_lat_lng_bounds* out_bounds
) -> mln_status {
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  if (out_bounds == nullptr) {
    set_thread_error("out_bounds must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_bounds = from_native_lat_lng_bounds(
    live.map->latLngBoundsForCameraUnwrapped(to_native_camera(*camera))
  );
  return MLN_STATUS_OK;
}

auto map_camera_for_lat_lng_bounds_start(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) -> mln_status {
  if (
    validate_lat_lng_bounds(bounds) != MLN_STATUS_OK ||
    validate_camera_fit_options(fit_options) != MLN_STATUS_OK
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto fit =
    fit_options == nullptr ? camera_fit_options_default() : *fit_options;
  const auto has_fit = fit_options != nullptr;
  return start_geometry_operation<&mln_map_camera_for_lat_lng_bounds>(
    map,
    [bounds, fit, has_fit](MapObject& live, mln_camera_options& result) {
      result = camera_options_default();
      return map_camera_for_lat_lng_bounds(
        live, bounds, has_fit ? &fit : nullptr, &result
      );
    },
    completion
  );
}

auto map_camera_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) -> mln_status {
  if (
    validate_lat_lng_array(coordinates, coordinate_count, false) !=
      MLN_STATUS_OK ||
    validate_camera_fit_options(fit_options) != MLN_STATUS_OK
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto copied =
    std::vector<mln_lat_lng>{coordinates, coordinates + coordinate_count};
  const auto fit =
    fit_options == nullptr ? camera_fit_options_default() : *fit_options;
  const auto has_fit = fit_options != nullptr;
  return start_geometry_operation<&mln_map_camera_for_lat_lngs>(
    map,
    [copied = std::move(copied), fit,
     has_fit](MapObject& live, mln_camera_options& result) {
      result = camera_options_default();
      return map_camera_for_lat_lngs(
        live, copied.data(), copied.size(), has_fit ? &fit : nullptr, &result
      );
    },
    completion
  );
}

auto map_camera_for_geometry_start(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) -> mln_status {
  const auto parsed = to_native_geometry(geometry);
  if (!parsed || geometry_lat_lngs(*parsed).empty()) {
    if (parsed) {
      set_thread_error("geometry must contain at least one coordinate");
    }
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_camera_fit_options(fit_options) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto* geometry_bytes = static_cast<const uint8_t*>(geometry.data);
  auto bytes =
    std::vector<uint8_t>{geometry_bytes, geometry_bytes + geometry.size};
  const auto fit =
    fit_options == nullptr ? camera_fit_options_default() : *fit_options;
  const auto has_fit = fit_options != nullptr;
  return start_geometry_operation<&mln_map_camera_for_geometry>(
    map,
    [bytes = std::move(bytes), fit,
     has_fit](MapObject& live, mln_camera_options& result) -> mln_status {
      result = camera_options_default();
      const auto view =
        mln_buffer_view{.data = bytes.data(), .size = bytes.size()};
      return map_camera_for_geometry(
        live, view, has_fit ? &fit : nullptr, &result
      );
    },
    completion
  );
}

template <auto Function>
auto start_bounds_for_camera(
  mln_map map, const mln_camera_options* camera, bool unwrapped,
  const mln_completion* completion
) -> mln_status {
  if (validate_camera_options(camera) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto copied = *camera;
  return start_geometry_operation<Function>(
    map,
    [copied, unwrapped](MapObject& live, mln_lat_lng_bounds& result) {
      return unwrapped
               ? map_lat_lng_bounds_for_camera_unwrapped(live, &copied, &result)
               : map_lat_lng_bounds_for_camera(live, &copied, &result);
    },
    completion
  );
}

auto map_lat_lng_bounds_for_camera_start(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) -> mln_status {
  return start_bounds_for_camera<&mln_map_lat_lng_bounds_for_camera>(
    map, camera, false, completion
  );
}

auto map_lat_lng_bounds_for_camera_unwrapped_start(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) -> mln_status {
  return start_bounds_for_camera<&mln_map_lat_lng_bounds_for_camera_unwrapped>(
    map, camera, true, completion
  );
}

auto map_set_bounds(MapObject& live, const mln_bound_options* options)
  -> mln_status {
  const auto options_status = validate_bound_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  // Native setBounds only applies optionals that are set, so this preserves
  // constraints omitted from options->fields.
  live.snapshot_config_dirty = true;
  live.snapshot_camera_dirty = true;
  live.map->setBounds(to_native_bound_options(*options));
  return MLN_STATUS_OK;
}

auto map_set_free_camera_options(
  MapObject& live, const mln_free_camera_options* options
) -> mln_status {
  const auto options_status = validate_free_camera_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  live.snapshot_camera_dirty = true;
  live.map->setFreeCameraOptions(to_native_free_camera(*options));
  return MLN_STATUS_OK;
}

auto map_set_projection_mode(
  mln_map map, const mln_projection_mode* mode, const mln_completion* completion
) -> mln_status {
  if (mode == nullptr || mode->size < sizeof(mln_projection_mode)) {
    set_thread_error("mode must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_PROJECTION_MODE_AXONOMETRIC) |
    MLN_PROJECTION_MODE_X_SKEW | MLN_PROJECTION_MODE_Y_SKEW;
  if ((mode->fields & ~known_fields) != 0U) {
    set_thread_error("mode fields contain unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    ((mode->fields & MLN_PROJECTION_MODE_X_SKEW) != 0U &&
     !std::isfinite(mode->x_skew)) ||
    ((mode->fields & MLN_PROJECTION_MODE_Y_SKEW) != 0U &&
     !std::isfinite(mode->y_skew))
  ) {
    set_thread_error("projection skew values must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto copied = mln::ProjectionMode{};
  if ((mode->fields & MLN_PROJECTION_MODE_AXONOMETRIC) != 0U) {
    copied.withAxonometric(mode->axonometric);
  }
  if ((mode->fields & MLN_PROJECTION_MODE_X_SKEW) != 0U) {
    copied.withXSkew(mode->x_skew);
  }
  if ((mode->fields & MLN_PROJECTION_MODE_Y_SKEW) != 0U) {
    copied.withYSkew(mode->y_skew);
  }
  return submit_map_command(
    map,
    [copied = std::move(copied)](MapObject& live) mutable -> mln_status {
      live.snapshot_config_dirty = true;
      live.snapshot_camera_dirty = true;
      live.map->setProjectionMode(std::move(copied));
      return MLN_STATUS_OK;
    },
    completion
  );
}

}  // namespace mln::core
