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

#include "bytes/buffer.hpp"
#include "completion/completion.hpp"
#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "handles/handle_table.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"
#include "style/style_value.hpp"

namespace {

enum class TileSourceOptionKind : uint8_t { Vector, Raster, RasterDEM };

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

  // A source with no release callback is not tracked.
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
  -> int32_t {
  switch (mode) {
    case mln::MapObserver::CameraChangeMode::Immediate:
      return MLN_CAMERA_CHANGE_MODE_IMMEDIATE;
    case mln::MapObserver::CameraChangeMode::Animated:
      return MLN_CAMERA_CHANGE_MODE_ANIMATED;
  }
  assert(false);
  return MLN_CAMERA_CHANGE_MODE_IMMEDIATE;
}

auto to_c_render_mode(mln::MapObserver::RenderMode mode) -> uint32_t {
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
    .stats = to_c_rendering_stats(status.renderingStats)
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

auto to_c_tile_operation(mln::TileOperation operation) -> uint32_t {
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

// Every callback tests the map's subscription mask before it builds anything,
// so an unselected event allocates no payload, message, or queue node.
class HeadlessObserver final : public mln::MapObserver {
 public:
  HeadlessObserver(
    mln_runtime runtime, mln_map map,
    std::shared_ptr<mln::core::MapEventState> event_state,
    std::shared_ptr<CallbackSourceRegistry> callback_sources
  )
      : runtime_(runtime),
        map_(map),
        event_state_(std::move(event_state)),
        callback_sources_(std::move(callback_sources)) {}

  void onCameraWillChange(CameraChangeMode mode) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE)) {
      return;
    }
    push(
      MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, to_c_camera_change_mode(mode)
    );
  }

  void onCameraIsChanging() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING);
  }

  void onCameraDidChange(CameraChangeMode mode) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE)) {
      return;
    }
    push(
      MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, to_c_camera_change_mode(mode)
    );
  }

  void onWillStartLoadingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED);
  }

  void onDidFinishLoadingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED);
  }

  // The failure text is map state that both style setters read, so it is
  // recorded whatever the mask selects.
  void onDidFailLoadingMap(
    mln::MapLoadError error, const std::string& message
  ) override {
    event_state_->style_load_failure = message;
    event_state_->style_load_failed = true;
    if (!selected(MLN_RUNTIME_EVENT_MAP_LOADING_FAILED)) {
      return;
    }
    push(
      MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, static_cast<int32_t>(error),
      message.c_str()
    );
  }

  // A style load can drop custom geometry sources that the previous style held.
  // The release callbacks that owes are map state rather than an event, so the
  // reconciliation runs whatever the mask selects.
  void onDidFinishLoadingStyle() override {
    // The event is queued before the reconciliation, and the registry is held
    // by a local share across it, because reconcile() runs host release
    // callbacks. Nothing may touch this observer or its members after host code
    // runs: a callback that destroys its map destroys this observer with it.
    if (selected(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)) {
      push(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED);
    }
    const auto sources = callback_sources_;
    sources->reconcile();
  }

  void onWillStartRenderingFrame() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED);
  }

  void onDidFinishRenderingFrame(const RenderFrameStatus& status) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME, render_frame_payload(status)
    );
  }

  void onWillStartRenderingMap() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED);
  }

  void onDidFinishRenderingMap(RenderMode mode) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
      MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP, render_map_payload(mode)
    );
  }

  void onDidBecomeIdle() override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_IDLE)) {
      return;
    }
    push(MLN_RUNTIME_EVENT_MAP_IDLE);
  }

  void onStyleImageMissing(const std::string& image_id) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING, MLN_RUNTIME_EVENT_PAYLOAD_NONE,
      mln::core::zeroed_event_payload(), 0, image_id
    );
  }

  void onTileAction(
    mln::TileOperation operation, const mln::OverscaledTileID& tile_id,
    const std::string& source_id
  ) override {
    if (!selected(MLN_RUNTIME_EVENT_MAP_TILE_ACTION)) {
      return;
    }
    push_payload(
      MLN_RUNTIME_EVENT_MAP_TILE_ACTION, MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
      tile_action_payload(operation, tile_id), 0, source_id
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
      push(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR);
    } catch (const std::exception& exception) {
      push(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, 0, exception.what());
    } catch (...) {
      push(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR, 0, "unknown render error");
    }
  }

 private:
  [[nodiscard]] auto selected(uint32_t type) const noexcept -> bool {
    return mln::core::event_selected(event_state_->mask, type);
  }

  auto push(uint32_t type, int32_t code = 0, const char* message = nullptr)
    -> void {
    mln::core::push_runtime_map_event(runtime_, map_, type, code, message);
  }

  auto push_payload(
    uint32_t type, uint32_t payload_type,
    const mln_runtime_event_payload& payload, int32_t code = 0,
    std::string message = {}
  ) -> void {
    mln::core::push_runtime_map_event_payload(
      runtime_, map_, type, payload_type, payload, code, std::move(message)
    );
  }

  mln_runtime runtime_;
  mln_map map_;
  std::shared_ptr<mln::core::MapEventState> event_state_;
  std::shared_ptr<CallbackSourceRegistry> callback_sources_;
};

}  // namespace mln::core

namespace {

using mln::core::HeadlessObserver;

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
        delegate_(delegate, mailbox_) {}

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
    const mln::gfx::RenderingStats& stats
  ) override {
    // The name carries three overloads; mln::Map::Impl implements only this
    // one.
    void (mln::RendererObserver::*method)(
      RenderMode, bool, bool, const mln::gfx::RenderingStats&
    ) = &mln::RendererObserver::onDidFinishRenderingFrame;
    delegate_.invoke(method, mode, repaint_needed, placement_changed, stats);
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
    mln_runtime runtime, mln_map map, mln::util::RunLoop& run_loop,
    std::shared_ptr<mln::core::MapEventState> event_state
  )
      : runtime_(runtime),
        map_(map),
        run_loop_(run_loop),
        event_state_(std::move(event_state)),
        thread_pool_(
          mln::Scheduler::GetBackground(), mln::util::SimpleIdentity{}
        ) {}

  void reset() override {
    const std::scoped_lock lock(latest_update_mutex_);
    latest_update_.reset();
    repaint_demand_ = false;
  }

  // mln::Map calls this once from its constructor on the runtime worker.
  void setObserver(mln::RendererObserver& observer) override {
    observer_ =
      std::make_unique<ForwardingRendererObserver>(run_loop_, observer);
  }

  // Store render state before publishing it to sessions and event consumers.
  void update(std::shared_ptr<mln::UpdateParameters> update) override {
    std::function<void()> publish;
    std::function<void()> publish_session;
    {
      const std::scoped_lock lock(latest_update_mutex_);
      latest_update_ = std::move(update);
      ++latest_update_generation_;
      repaint_demand_ = true;
      publish = publish_;
      publish_session = session_publish_;
    }
    if (publish) {
      publish();
    }
    if (publish_session) {
      publish_session();
    }
    if (
      mln::core::event_selected(
        event_state_->mask, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
      )
    ) {
      mln::core::push_runtime_map_event(
        runtime_, map_, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
      );
    }
  }

  [[nodiscard]] auto latest_update() const
    -> std::shared_ptr<mln::UpdateParameters> {
    const std::scoped_lock lock(latest_update_mutex_);
    return latest_update_;
  }
  // One lock hold keeps the pair coherent: a frame result must report the
  // generation of the update that was actually rendered.
  [[nodiscard]] auto latest_update_snapshot(uint64_t& out_generation) const
    -> std::shared_ptr<mln::UpdateParameters> {
    const std::scoped_lock lock(latest_update_mutex_);
    out_generation = latest_update_generation_;
    return latest_update_;
  }
  auto set_publish_callback(std::function<void()> publish) -> void {
    const std::scoped_lock lock(latest_update_mutex_);
    publish_ = std::move(publish);
  }
  auto set_session_publish_callback(std::function<void()> publish) -> void {
    const std::scoped_lock lock(latest_update_mutex_);
    session_publish_ = std::move(publish);
  }

  [[nodiscard]] auto latest_update_generation() const -> uint64_t {
    const std::scoped_lock lock(latest_update_mutex_);
    return latest_update_generation_;
  }

  [[nodiscard]] auto repaint_demand() const -> bool {
    const std::scoped_lock lock(latest_update_mutex_);
    return repaint_demand_;
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
  mln_runtime runtime_;
  mln_map map_;
  mln::util::RunLoop& run_loop_;
  std::shared_ptr<mln::core::MapEventState> event_state_;
  std::unique_ptr<ForwardingRendererObserver> observer_;
  mln::TaggedScheduler thread_pool_;
  mutable std::mutex latest_update_mutex_;
  std::shared_ptr<mln::UpdateParameters> latest_update_;
  std::function<void()> publish_;
  std::function<void()> session_publish_;
  uint64_t latest_update_generation_ = 0;
  bool repaint_demand_ = false;
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

  // Validated here as well as in the setter, so a mask this library cannot
  // honour is rejected wherever it arrives.
  constexpr auto known_mask_bits =
    static_cast<uint64_t>(MLN_RUNTIME_EVENT_MASK_ALL);
  if ((options->event_mask & ~known_mask_bits) != 0U) {
    mln::core::set_thread_error(
      "mln_map_options.event_mask contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (
    options->initial_extent.width == 0 || options->initial_extent.height == 0 ||
    !std::isfinite(options->initial_extent.scale_factor) ||
    options->initial_extent.scale_factor <= 0
  ) {
    mln::core::set_thread_error(
      "initial extent dimensions and scale factor must be positive"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
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

auto to_native_map_mode(uint32_t mode) -> mln::MapMode {
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

auto is_still_map_mode(uint32_t mode) -> bool {
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
    return "unknown still-image request error";
  }
}

auto validate_lat_lng(mln_lat_lng coordinate) -> mln_status;
auto validate_edge_insets(mln_edge_insets padding) -> mln_status;
auto validate_screen_point(mln_screen_point point) -> mln_status;

auto validate_camera_options(const mln_camera_options* camera) -> mln_status {
  if (camera == nullptr) {
    mln::core::set_thread_error("camera must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (camera->size < sizeof(mln_camera_options)) {
    mln::core::set_thread_error("mln_camera_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

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
    const auto status = validate_lat_lng(
      mln_lat_lng{.latitude = camera->latitude, .longitude = camera->longitude}
    );
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
  if (animation->size < sizeof(mln_animation_options)) {
    mln::core::set_thread_error("mln_animation_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
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
        "animation easing x values must be within [0, 1] and all easing values "
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
    result.withCenter(mln::LatLng{camera.latitude, camera.longitude});
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
    result.latitude = camera.center->latitude();
    result.longitude = camera.center->longitude();
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

auto camera_transition_finished_payload(uint64_t transition_id)
  -> mln_runtime_event_payload {
  auto payload = mln::core::zeroed_event_payload();
  payload.camera_transition_finished =
    mln_runtime_event_camera_transition_finished{
      .transition_id = transition_id
    };
  return payload;
}

// MapLibre Native owns the returned AnimationOptions for the transition
// lifetime and invokes transitionFinishFn on the runtime worker. Event
// publication stops for a closed map. The lambda holds event state by value, so
// it can still inspect the selected mask after map close.
auto to_native_animation(
  mln_runtime runtime, mln_map map,
  const std::shared_ptr<mln::core::MapEventState>& event_state,
  const mln_animation_options* animation
) -> mln::AnimationOptions {
  auto result = mln::AnimationOptions{};
  if (animation == nullptr) {
    return result;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0U) {
    result.transitionFinishFn = [runtime, map, event_state,
                                 transition_id = animation->transition_id] {
      if (!mln::core::event_selected(
            event_state->mask, MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED
          )) {
        return;
      }
      mln::core::push_runtime_map_event_payload(
        runtime, map, MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED,
        MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED,
        camera_transition_finished_payload(transition_id)
      );
    };
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_DURATION) != 0U) {
    result.duration = duration_from_milliseconds(animation->duration_ms);
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0U) {
    result.velocity = animation->velocity;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0U) {
    result.minZoom = animation->min_zoom;
  }
  if ((animation->fields & MLN_ANIMATION_OPTION_EASING) != 0U) {
    const auto easing = animation->easing;
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

auto to_native_north_orientation(uint32_t orientation)
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
  -> uint32_t {
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

auto to_native_constrain_mode(uint32_t mode) -> mln::ConstrainMode {
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

auto from_native_constrain_mode(mln::ConstrainMode mode) -> uint32_t {
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

auto to_native_viewport_mode(uint32_t mode) -> mln::ViewportMode {
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

auto from_native_viewport_mode(mln::ViewportMode mode) -> uint32_t {
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

auto to_native_tile_lod_mode(uint32_t mode) -> mln::TileLodMode {
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

auto from_native_tile_lod_mode(mln::TileLodMode mode) -> uint32_t {
  switch (mode) {
    case mln::TileLodMode::Distance:
      return MLN_TILE_LOD_MODE_DISTANCE;
    case mln::TileLodMode::Default:
      return MLN_TILE_LOD_MODE_DEFAULT;
  }
  assert(false);
  return MLN_TILE_LOD_MODE_DEFAULT;
}

auto from_native_projection_mode(const mln::ProjectionMode& mode)
  -> mln_projection_mode {
  auto result = mln::core::projection_mode_default();
  if (mode.axonometric) {
    result.fields |= MLN_PROJECTION_MODE_AXONOMETRIC;
    result.axonometric = *mode.axonometric;
  }
  if (mode.xSkew) {
    result.fields |= MLN_PROJECTION_MODE_X_SKEW;
    result.x_skew = *mode.xSkew;
  }
  if (mode.ySkew) {
    result.fields |= MLN_PROJECTION_MODE_Y_SKEW;
    result.y_skew = *mode.ySkew;
  }
  return result;
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

auto to_native_screen_points(const mln_screen_point* points, size_t point_count)
  -> std::vector<mln::ScreenCoordinate> {
  auto result = std::vector<mln::ScreenCoordinate>{};
  result.reserve(point_count);
  const auto point_span =
    std::span<const mln_screen_point>{points, point_count};
  for (const auto point : point_span) {
    result.emplace_back(to_native_screen_point(point));
  }
  return result;
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

}  // namespace

namespace mln::core {
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

class PendingMapResult final {
 public:
  explicit PendingMapResult(mln_map value) : value_(value) {}
  ~PendingMapResult();
  PendingMapResult(const PendingMapResult&) = delete;
  PendingMapResult(PendingMapResult&&) = delete;
  auto operator=(const PendingMapResult&) -> PendingMapResult& = delete;
  auto operator=(PendingMapResult&&) -> PendingMapResult& = delete;

  [[nodiscard]] auto value() const noexcept -> mln_map { return value_; }
  auto transfer() noexcept -> void { value_ = MLN_HANDLE_NULL; }

 private:
  mln_map value_ = MLN_HANDLE_NULL;
};

}  // namespace mln::core

namespace mln::core {

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
  auto operation = std::exchange(live->still_image_operation, {});
  if (auto release = std::exchange(live->still_image_release_submission, {})) {
    release();
  }
  if (error) {
    const auto message = exception_message(error);
    if (operation) {
      operation->complete(
        MLN_STATUS_NATIVE_ERROR, message, std::any{std::monostate{}}
      );
    }
    if (
      event_selected(
        live->event_state->mask, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED
      )
    ) {
      push_runtime_map_event(
        live->runtime, map, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED, 0,
        message.c_str()
      );
    }
    return;
  }
  if (operation) {
    operation->complete(MLN_STATUS_OK, {}, std::any{std::monostate{}});
  }
  if (
    event_selected(
      live->event_state->mask, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED
    )
  ) {
    push_runtime_map_event(
      live->runtime, map, MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED
    );
  }
}

// The caller holds the map handle table's mutex, so it can act on the result
// without the handle being retired in between.
auto validate_map_live_locked(mln_map map, MapObject*& out_map) -> mln_status {
  out_map = handle_table<MapObject>().resolve_locked(map);
  return out_map == nullptr ? MLN_STATUS_INVALID_ARGUMENT : MLN_STATUS_OK;
}

// Same locking contract as validate_map_live_locked().
auto validate_map_locked(mln_map map, MapObject*& out_map) -> mln_status {
  return validate_map_live_locked(map, out_map);
}

auto publish_map_snapshot(MapObject& live) -> uint64_t {
  const auto options = live.map->getMapOptions();
  auto snapshot = mln_map_snapshot{
    .size = sizeof(mln_map_snapshot),
    .debug_options = from_native_debug_options(live.map->getDebug()),
    .generation = live.next_snapshot_generation++,
    .camera = from_native_camera(live.map->getCameraOptions()),
    .logical_extent = live.logical_extent,
    .projection_mode =
      from_native_projection_mode(live.map->getProjectionMode()),
    .viewport =
      {.size = sizeof(mln_map_viewport_options),
       .fields =
         static_cast<uint32_t>(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) |
         MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE |
         MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE |
         MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET,
       .north_orientation =
         from_native_north_orientation(options.northOrientation()),
       .constrain_mode = from_native_constrain_mode(options.constrainMode()),
       .viewport_mode = from_native_viewport_mode(options.viewportMode()),
       .frustum_offset = from_native_edge_insets(live.map->getFrustumOffset())},
    .fully_loaded = live.map->isFullyLoaded(),
    .rendering_stats_view_enabled = live.map->isRenderingStatsViewEnabled(),
    .repaint_demand = live.frontend->repaint_demand(),
    .reserved_flags = 0,
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
    .free_camera = from_native_free_camera(live.map->getFreeCameraOptions())
  };
  const auto generation = snapshot.generation;
  {
    const std::scoped_lock lock(live.snapshot_mutex);
    live.snapshot = snapshot;
  }
  return generation;
}

}  // namespace

auto map_options_default() noexcept -> mln_map_options {
  return mln_map_options{
    .size = sizeof(mln_map_options),
    .initial_extent =
      {.width = default_map_width,
       .height = default_map_height,
       .scale_factor = default_scale_factor},
    .map_mode = MLN_MAP_MODE_CONTINUOUS,
    .fast_pfor_enabled = false,
    .event_mask = MLN_RUNTIME_EVENT_MASK_ALL
  };
}

auto camera_options_default() noexcept -> mln_camera_options {
  return mln_camera_options{
    .size = sizeof(mln_camera_options),
    .fields = 0,
    .latitude = 0,
    .longitude = 0,
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
    .size = sizeof(mln_animation_options),
    .fields = 0,
    .duration_ms = 0,
    .velocity = 0,
    .min_zoom = 0,
    .easing = {.x1 = 0, .y1 = 0, .x2 = 0.25, .y2 = 1},
    .transition_id = 0
  };
}

auto camera_delta_default() noexcept -> mln_camera_delta {
  return mln_camera_delta{
    .size = sizeof(mln_camera_delta),
    .kind = MLN_CAMERA_DELTA_MOVE,
    .offset = {},
    .amount = 0,
    .has_anchor = false,
    .anchor = {},
    .animation = animation_options_default()
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

auto validate_map(mln_map map, MapObject*& out_map) -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  return validate_map_locked(map, out_map);
}

namespace {

struct MapSubmissionContext {
  std::shared_ptr<MapObject> map;
  std::shared_ptr<ControlLease> control;
  std::shared_ptr<RuntimeObject> runtime;
};

auto acquire_map_submission(mln_map map, MapSubmissionContext& out_context)
  -> mln_status {
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto guard = ControlLease{&live->control};
  auto control = std::make_shared<ControlLease>(std::move(guard));
  auto runtime = lease_runtime(live->runtime);
  if (runtime == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_context = MapSubmissionContext{
    .map = std::move(live),
    .control = std::move(control),
    .runtime = std::move(runtime),
  };
  return MLN_STATUS_OK;
}

}  // namespace

auto submit_map_command(
  mln_map map, std::function<mln_status()> work,
  const mln_completion* completion
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto context = MapSubmissionContext{};
  const auto acquire_status = acquire_map_submission(map, context);
  if (acquire_status != MLN_STATUS_OK) {
    return acquire_status;
  }
  auto live = std::move(context.map);
  auto map_lease = std::move(context.control);
  auto runtime = std::move(context.runtime);
  auto completion_state = std::make_shared<Completion>(*completion);
  return submit_runtime_command(
    runtime,
    [live = std::move(live), map_lease = std::move(map_lease), completion_state,
     work = std::move(work)](uint64_t) mutable -> void {
      clear_thread_error();
      auto status = MLN_STATUS_NATIVE_ERROR;
      auto message = std::string{};
      auto generation = uint64_t{0};
      try {
        status = std::invoke(std::move(work));
        message = thread_last_error_message();
        // Committed commands republish so snapshot reads observe the commit.
        if (status == MLN_STATUS_OK) {
          generation = publish_map_snapshot(*live);
        }
      } catch (const std::exception& exception) {
        status = MLN_STATUS_NATIVE_ERROR;
        generation = 0;
        message = exception.what();
      } catch (...) {
        status = MLN_STATUS_NATIVE_ERROR;
        generation = 0;
        message = "map command failed";
      }
      complete_command(
        completion_state,
        status == MLN_STATUS_OK ? MLN_COMMAND_DISPOSITION_COMMITTED
                                : MLN_COMMAND_DISPOSITION_FAILED,
        status, generation, std::move(message)
      );
    },
    completion_state
  );
}

auto start_style_operation(
  mln_map map, StyleOperationKind kind, StyleWork work,
  const mln_completion* completion
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto completion_state = std::make_shared<Completion>(*completion);
  auto result_callback = [completion_state, kind](
                           mln_status status, std::string diagnostic,
                           std::any result
                         ) mutable {
    auto shared = std::any_cast<std::shared_ptr<StyleOperationResult>>(&result);
    if (status != MLN_STATUS_OK || shared == nullptr || *shared == nullptr) {
      complete(
        completion_state,
        status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
        status == MLN_STATUS_OK ? "style operation produced an invalid result"
                                : std::move(diagnostic)
      );
      return;
    }
    completion_state->resolve([kind, result = std::move(*shared)](
                                const mln_completion& descriptor
                              ) {
      const void* value = nullptr;
      auto count = std::size_t{0};
      auto view = mln_buffer_view{};
      auto views = std::vector<mln_buffer_view>{};
      auto stretches = mln_style_image_stretches_result{};
      auto source = mln_style_source_result{};
      auto layer = mln_style_layer_result{};
      auto image = mln_style_image_result{};
      switch (kind) {
        case StyleOperationKind::SourceInfo:
          if (result->found) {
            views.reserve(result->strings.size());
            for (const auto& string : result->strings) {
              views.push_back({.data = string.data(), .size = string.size()});
            }
            source = {
              .size = sizeof(mln_style_source_result),
              .reserved = 0,
              .info = result->source_info,
              .attribution =
                {.data = result->attribution.data(),
                 .size = result->attribution.size()},
              .url = {.data = result->url.data(), .size = result->url.size()},
              .tile_urls = views.data(),
              .tile_url_count = views.size()
            };
            value = &source;
            count = 1;
          }
          break;
        case StyleOperationKind::LayerInfo:
          if (result->found) {
            layer = {
              .size = sizeof(mln_style_layer_result),
              .reserved = 0,
              .info = result->layer_info,
              .source_id =
                {.data = result->source_id.data(),
                 .size = result->source_id.size()},
              .source_layer = {
                .data = result->source_layer.data(),
                .size = result->source_layer.size()
              }
            };
            value = &layer;
            count = 1;
          }
          break;
        case StyleOperationKind::ImageInfo:
          if (result->found) {
            image = {
              .size = sizeof(mln_style_image_result),
              .reserved = 0,
              .info = result->image_info,
              .pixels =
                {.data = result->bytes.data(), .size = result->bytes.size()},
              .stretch_x = result->stretch_x.data(),
              .stretch_x_count = result->stretch_x.size(),
              .stretch_y = result->stretch_y.data(),
              .stretch_y_count = result->stretch_y.size()
            };
            value = &image;
            count = 1;
          }
          break;
        case StyleOperationKind::TransitionOptions:
          value = &result->transition_options;
          count = 1;
          break;
        case StyleOperationKind::SourceIds:
        case StyleOperationKind::LayerIds:
          views.reserve(result->strings.size());
          for (const auto& string : result->strings) {
            views.push_back({.data = string.data(), .size = string.size()});
          }
          value = views.data();
          count = views.size();
          break;
        case StyleOperationKind::SourceTileUrls:
          if (result->found) {
            views.reserve(result->strings.size());
            for (const auto& string : result->strings) {
              views.push_back({.data = string.data(), .size = string.size()});
            }
            value = views.data();
            count = views.size();
          }
          break;
        case StyleOperationKind::ImageStretches:
          if (result->found) {
            stretches = mln_style_image_stretches_result{
              .size = sizeof(mln_style_image_stretches_result),
              .reserved = 0,
              .stretch_x = result->stretch_x.data(),
              .stretch_x_count = result->stretch_x.size(),
              .stretch_y = result->stretch_y.data(),
              .stretch_y_count = result->stretch_y.size(),
            };
            value = &stretches;
            count = 1;
          }
          break;
        case StyleOperationKind::ImageCoordinates:
          value = result->found ? result->coordinates.data() : nullptr;
          count = result->found ? result->coordinates.size() : 0;
          break;
        case StyleOperationKind::SourceAttribution:
        case StyleOperationKind::SourceUrl:
        case StyleOperationKind::LayerJson:
        case StyleOperationKind::LightProperty:
        case StyleOperationKind::LayerProperty:
          if (result->found) {
            view = {.data = result->bytes.data(), .size = result->bytes.size()};
            value = &view;
            count = 1;
          }
          break;
        default:
          if (
            result->found || (kind != StyleOperationKind::ImagePixels &&
                              kind != StyleOperationKind::LayerFilter)
          ) {
            view = {.data = result->bytes.data(), .size = result->bytes.size()};
            value = &view;
            count = 1;
          }
          break;
      }
      invoke_completion(
        descriptor, MLN_STATUS_OK, MLN_COMMAND_DISPOSITION_COMMITTED, 0, {},
        value, count
      );
    });
  };
  auto context = MapSubmissionContext{};
  const auto acquire_status = acquire_map_submission(map, context);
  if (acquire_status != MLN_STATUS_OK) return acquire_status;
  auto state = std::make_shared<OperationObject>(std::move(result_callback));
  const auto submission = submit_runtime_operation(
    context.runtime, state,
    [map = std::move(context.map), control = std::move(context.control), state,
     work = std::move(work)]() mutable -> void {
      static_cast<void>(map);
      static_cast<void>(control);
      auto result = std::make_shared<StyleOperationResult>();
      clear_thread_error();
      try {
        const auto status = std::invoke(std::move(work), *result);
        state->complete(status, thread_last_error_message(), std::any{result});
      } catch (const std::exception& exception) {
        state->complete(MLN_STATUS_NATIVE_ERROR, exception.what(), result);
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, "style operation failed", result
        );
      }
    }
  );
  if (submission == MLN_STATUS_OK)
    completion_state->accept();
  else
    completion_state->reject();
  return submission;
}

auto start_geometry_operation(
  mln_map map, GeometryOperationKind kind, GeometryWork work,
  const mln_completion* completion
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto completion_state = std::make_shared<Completion>(*completion);
  auto context = MapSubmissionContext{};
  const auto acquire_status = acquire_map_submission(map, context);
  if (acquire_status != MLN_STATUS_OK) return acquire_status;
  auto state = std::make_shared<OperationObject>(
    [completion_state,
     kind](mln_status status, std::string diagnostic, std::any result) mutable {
      auto* value = std::any_cast<GeometryOperationResult>(&result);
      if (status != MLN_STATUS_OK || value == nullptr) {
        complete(
          completion_state,
          status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
          status == MLN_STATUS_OK
            ? "geometry operation produced an invalid result"
            : std::move(diagnostic)
        );
        return;
      }
      completion_state->resolve(
        [kind, value = std::move(*value)](const mln_completion& descriptor) {
          const void* pointer = nullptr;
          auto count = std::size_t{1};
          switch (kind) {
            case GeometryOperationKind::CameraForBounds:
            case GeometryOperationKind::CameraForCoordinates:
            case GeometryOperationKind::CameraForGeometry:
              pointer = &value.camera;
              break;
            case GeometryOperationKind::BoundsForCamera:
            case GeometryOperationKind::UnwrappedBoundsForCamera:
              pointer = &value.bounds;
              break;
            case GeometryOperationKind::PixelForCoordinate:
              pointer = &value.point;
              break;
            case GeometryOperationKind::CoordinateForPixel:
              pointer = &value.coordinate;
              break;
            case GeometryOperationKind::PixelsForCoordinates:
              pointer = value.points.data();
              count = value.points.size();
              break;
            case GeometryOperationKind::CoordinatesForPixels:
              pointer = value.coordinates.data();
              count = value.coordinates.size();
              break;
          }
          invoke_completion(
            descriptor, MLN_STATUS_OK, MLN_COMMAND_DISPOSITION_COMMITTED, 0, {},
            pointer, count
          );
        }
      );
    }
  );
  const auto submission = submit_runtime_operation(
    context.runtime, state,
    [map = std::move(context.map), control = std::move(context.control), state,
     work = std::move(work)]() mutable {
      static_cast<void>(map);
      static_cast<void>(control);
      auto result = GeometryOperationResult{};
      clear_thread_error();
      try {
        const auto status = std::invoke(std::move(work), result);
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
    }
  );
  if (submission == MLN_STATUS_OK)
    completion_state->accept();
  else
    completion_state->reject();
  return submission;
}

namespace {

// Runs work against a live projection on the calling thread, serialized with
// every other projection call, including close, by the per-projection mutex.
template <typename Work>
auto with_projection(mln_map_projection projection, Work work) -> mln_status {
  auto live = handle_table<MapProjectionObject>().lease(projection);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock call_lock(live->call_mutex);
  if (live->projection == nullptr) {
    set_handle_fault_error(
      HandleTraits<MapProjectionObject>::kind, projection, HandleFault::Stale
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  work(*live->projection);
  return MLN_STATUS_OK;
}

}  // namespace

namespace {
auto ensure_map_teardown_lane() -> void;
}

auto create_map(
  mln_runtime runtime, const mln_map_options* options, mln_map* out_map
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

  RuntimeObject* live_runtime = nullptr;
  const auto runtime_status = validate_runtime(runtime, live_runtime);
  if (runtime_status != MLN_STATUS_OK) {
    return runtime_status;
  }
  ensure_map_teardown_lane();

  const auto retain_status = retain_runtime_map(runtime);
  if (retain_status != MLN_STATUS_OK) {
    return retain_status;
  }
  auto retain_guard = RuntimeMapRetainGuard{runtime};

  const auto effective = options == nullptr ? map_options_default() : *options;
  auto owned_map = std::make_shared<MapObject>();
  // Every allocation this function owns happens before the handle is published,
  // so a throw here cannot leave a registered map the caller has no handle to
  // destroy.
  auto event_state = std::make_shared<MapEventState>();
  auto source_registry = std::make_shared<CallbackSourceRegistry>();
  // Publish the handle first, so the observer and frontend capture an id that
  // already resolves.
  const auto handle = handle_table<MapObject>().insert(owned_map);
  owned_map->runtime = runtime;
  owned_map->runtime_state = lease_runtime(runtime);
  owned_map->map_mode = effective.map_mode;
  owned_map->logical_extent = effective.initial_extent;
  owned_map->event_state = std::move(event_state);
  owned_map->callback_sources = std::move(source_registry);
  owned_map->event_state->mask.store(
    effective.event_mask, std::memory_order_relaxed
  );
  try {
    // Registering allocates, so it belongs inside the scope that unpublishes
    // the handle on failure. Nothing before this point queues an event, and the
    // observer and frontend below are the first producers that need it.
    register_runtime_map_events(runtime, handle, owned_map->event_state);
    owned_map->observer = std::make_unique<HeadlessObserver>(
      runtime, handle, owned_map->event_state, owned_map->callback_sources
    );
    owned_map->frontend = std::make_unique<HeadlessFrontend>(
      runtime, handle, runtime_run_loop(live_runtime), owned_map->event_state
    );

    auto map_options = mln::MapOptions{};
    map_options.withMapMode(to_native_map_mode(effective.map_mode))
      .withSize(
        mln::Size{
          effective.initial_extent.width, effective.initial_extent.height
        }
      )
      .withPixelRatio(static_cast<float>(effective.initial_extent.scale_factor))
      .withFastPFOREnabled(effective.fast_pfor_enabled);
    owned_map->map = std::make_unique<mln::Map>(
      *owned_map->frontend, *owned_map->observer, map_options,
      resource_options_for_runtime(runtime)
    );
    owned_map->callback_sources->attach(*owned_map->map);
    owned_map->frontend->set_publish_callback(
      [weak = std::weak_ptr<MapObject>{owned_map}]() -> void {
        if (const auto locked = weak.lock(); locked && locked->map) {
          publish_map_snapshot(*locked);
        }
      }
    );
    publish_map_snapshot(*owned_map);

  } catch (...) {
    static_cast<void>(handle_table<MapObject>().remove(handle));
    unregister_runtime_map_events(runtime, handle);
    throw;
  }
  *out_map = handle;
  retain_guard.dismiss();
  return MLN_STATUS_OK;
}
auto create_map_start(
  mln_runtime runtime, const mln_map_options* options,
  const mln_completion* completion
) -> mln_status {
  const auto options_status = validate_map_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  auto runtime_state = lease_runtime(runtime);
  if (runtime_state == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto reserve_status = retain_runtime_map(runtime);
  if (reserve_status != MLN_STATUS_OK) {
    return reserve_status;
  }
  const auto effective = options == nullptr ? map_options_default() : *options;
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) {
    release_runtime_map(runtime);
    return completion_status;
  }
  auto completion_state = std::make_shared<Completion>(*completion);
  auto state =
    std::make_shared<OperationObject>([completion_state](
                                        mln_status status,
                                        std::string diagnostic, std::any result
                                      ) mutable {
      if (status != MLN_STATUS_OK) {
        complete(completion_state, status, std::move(diagnostic));
        return;
      }
      auto* pending = std::any_cast<std::shared_ptr<PendingMapResult>>(&result);
      if (pending == nullptr || *pending == nullptr) {
        complete(
          completion_state, MLN_STATUS_NATIVE_ERROR,
          "map creation produced an invalid result"
        );
        return;
      }
      const auto map = (*pending)->value();
      (*pending)->transfer();
      complete_value(completion_state, MLN_STATUS_OK, {}, map);
    });
  const auto submit_status = submit_runtime_operation(
    runtime_state, state, [runtime, effective, state]() mutable -> void {
      auto result = mln_map{MLN_HANDLE_NULL};
      auto status = MLN_STATUS_NATIVE_ERROR;
      auto diagnostic = std::string{};
      try {
        status = create_map(runtime, &effective, &result);
      } catch (...) {
        diagnostic = exception_message(std::current_exception());
      }
      // Drop the creation reservation before publishing completion. Once the
      // result is observable, the map itself is the runtime's only child and
      // callers may release the map and runtime back to back.
      release_runtime_map(runtime);
      if (status == MLN_STATUS_OK) {
        state->complete(
          status, {}, std::any{std::make_shared<PendingMapResult>(result)}
        );
      } else {
        state->complete(
          status,
          diagnostic.empty() ? "map creation failed" : std::move(diagnostic), {}
        );
      }
    }
  );
  if (submit_status != MLN_STATUS_OK) {
    completion_state->reject();
    release_runtime_map(runtime);
  } else {
    completion_state->accept();
  }
  return submit_status;
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
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr || live->control.is_closing()) {
    return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT
                           : MLN_STATUS_INVALID_STATE;
  }
  const std::scoped_lock lock(live->snapshot_mutex);
  *out_snapshot = live->snapshot;
  return MLN_STATUS_OK;
}

auto map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (
    completion_status != MLN_STATUS_OK || extent.width == 0 ||
    extent.height == 0 || !std::isfinite(extent.scale_factor) ||
    extent.scale_factor <= 0
  ) {
    if (completion_status == MLN_STATUS_OK)
      set_thread_error("extent must be valid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto submission = std::make_shared<ControlLease>(&live->control);
  auto completion_state = std::make_shared<Completion>(*completion);
  const auto status = submit_runtime_command(
    live->runtime_state,
    [live, submission = std::move(submission), completion_state,
     extent](uint64_t sequence) mutable -> void {
      if (sequence != live->latest_resize_submission.load()) {
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_SUPERSEDED, MLN_STATUS_OK
        );
        return;
      }
      try {
        live->logical_extent = extent;
        live->map->setSize(mln::Size{extent.width, extent.height});
        const auto generation = publish_map_snapshot(*live);
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK,
          generation
        );
      } catch (...) {
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_FAILED,
          MLN_STATUS_NATIVE_ERROR, 0,
          exception_message(std::current_exception())
        );
      }
    },
    completion_state, &live->latest_resize_submission
  );
  return status;
}

namespace {
auto discarded_completion() noexcept -> mln_completion;

// Serializes blocking map destruction away from runtime executors. One shared
// worker is reserved before a map creates its own worker pool, so close never
// depends on creating a thread after that pool is saturated.
class MapTeardownLane {
 public:
  MapTeardownLane() {
    std::thread([this]() noexcept -> void { run(); }).detach();
  }

  auto submit(std::function<void()> teardown) -> void {
    {
      const std::scoped_lock lock(mutex_);
      pending_.push_back(std::move(teardown));
    }
    condition_.notify_one();
  }

 private:
  auto run() noexcept -> void {
    for (;;) {
      auto teardown = std::function<void()>{};
      {
        auto lock = std::unique_lock{mutex_};
        condition_.wait(lock, [this]() noexcept -> bool {
          return !pending_.empty();
        });
        teardown = std::move(pending_.front());
        pending_.pop_front();
      }
      try {
        teardown();
      } catch (...) {
      }
    }
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<std::function<void()>> pending_;
};

auto map_teardown_lane() -> MapTeardownLane& {
  // Process lifetime avoids a static-destruction race with the retiring lane.
  static auto* lane = new MapTeardownLane{};
  return *lane;
}

auto ensure_map_teardown_lane() -> void {
  static_cast<void>(map_teardown_lane());
}
}  // namespace

auto release_map(mln_map map, const mln_completion* completion) -> mln_status {
  CompletionOperation teardown;
  const auto completion_status =
    create_completion_operation(completion, {}, teardown);
  if (completion_status != MLN_STATUS_OK) {
    return completion_status;
  }
  auto owned_map = handle_table<MapObject>().lease(map);
  if (owned_map == nullptr) {
    teardown.completion->reject();
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  {
    const std::scoped_lock lock(handle_table<MapObject>().mutex());
    auto* live = handle_table<MapObject>().resolve_locked(map);
    if (live == nullptr || live != owned_map.get()) {
      teardown.completion->reject();
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (live->render_target_session != nullptr) {
      teardown.completion->reject();
      set_thread_error("map still has an attached render session");
      return MLN_STATUS_INVALID_STATE;
    }
    const auto close_status = live->control.begin_close();
    if (close_status != MLN_STATUS_OK) {
      teardown.completion->reject();
      return close_status;
    }
  }
  struct CloseState {
    std::mutex mutex;
    bool ordered = false;
    bool drained = false;
    bool scheduled = false;
    mln_map map = MLN_HANDLE_NULL;
    std::shared_ptr<MapObject> owned_map;
    std::shared_ptr<OperationObject> operation;
  };
  auto close = std::shared_ptr<CloseState>{};
  try {
    close = std::make_shared<CloseState>();
  } catch (...) {
    owned_map->control.abort_close();
    teardown.completion->reject();
    set_thread_error("map release could not allocate its teardown gate");
    return MLN_STATUS_NATIVE_ERROR;
  }
  close->map = map;
  close->owned_map = owned_map;
  close->operation = teardown.operation;
  auto schedule_if_ready = [close]() noexcept -> void {
    auto schedule = false;
    {
      const std::scoped_lock lock(close->mutex);
      if (close->ordered && close->drained && !close->scheduled) {
        close->scheduled = true;
        schedule = true;
      }
    }
    if (!schedule) return;
    try {
      map_teardown_lane().submit([close]() mutable {
        auto owned = std::move(close->owned_map);
        owned->callback_sources->detach();
        owned->frontend->close_renderer_observer();
        // Retire the map before reporting completion, so it can no longer
        // reach host callback state. Browser backends can require main-thread
        // service while their worker pool shuts down, so that backend cleanup
        // continues after the public retirement boundary.
        owned->map.reset();
        owned->callback_sources->release_all();
        {
          const std::scoped_lock event_lock(
            owned->runtime_state->event_queue->mutex
          );
          owned->runtime_state->event_queue->event_maps.erase(close->map);
        }
        close->operation->complete(
          MLN_STATUS_OK, {}, std::any{std::monostate{}}
        );
        owned->frontend->shutdown_thread_pool();
        owned.reset();
      });
    } catch (...) {
      close->operation->complete(
        MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()), {}
      );
    }
  };
  const auto submit_status = submit_runtime_operation(
    owned_map->runtime_state, teardown.operation,
    [close, schedule_if_ready]() mutable -> void {
      if (close->owned_map->still_image_operation != nullptr) {
        close->owned_map->still_image_operation->complete(
          MLN_STATUS_CANCELLED, "map closed before still image completed", {}
        );
      }
      if (
        auto release =
          std::exchange(close->owned_map->still_image_release_submission, {})
      ) {
        release();
      }
      {
        const std::scoped_lock lock(close->mutex);
        close->ordered = true;
      }
      schedule_if_ready();
    }
  );
  if (submit_status != MLN_STATUS_OK) {
    owned_map->control.abort_close();
    teardown.completion->reject();
    return submit_status;
  }
  {
    const std::scoped_lock lock(handle_table<MapObject>().mutex());
    static_cast<void>(handle_table<MapObject>().remove_locked(map));
  }
  release_runtime_map(owned_map->runtime);
  owned_map->control.notify_when_drained([close, schedule_if_ready]() {
    {
      const std::scoped_lock lock(close->mutex);
      close->drained = true;
    }
    schedule_if_ready();
  });
  teardown.completion->accept();
  return MLN_STATUS_OK;
}
PendingMapResult::~PendingMapResult() {
  if (value_ == MLN_HANDLE_NULL) {
    return;
  }
  try {
    const auto completion = discarded_completion();
    static_cast<void>(release_map(value_, &completion));
  } catch (...) {
  }
}

auto map_request_repaint(mln_map map, const mln_completion* completion)
  -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto submission = std::make_shared<ControlLease>(&live->control);
  auto completion_state = std::make_shared<Completion>(*completion);
  return submit_runtime_command(
    live->runtime_state,
    [live, submission = std::move(submission),
     completion_state](uint64_t) mutable -> void {
      if (live->map_mode != MLN_MAP_MODE_CONTINUOUS) {
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_FAILED,
          MLN_STATUS_INVALID_STATE, 0, "map is not in continuous mode"
        );
        return;
      }
      try {
        live->map->triggerRepaint();
        const auto generation = publish_map_snapshot(*live);
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK,
          generation
        );
      } catch (...) {
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_FAILED,
          MLN_STATUS_NATIVE_ERROR, 0,
          exception_message(std::current_exception())
        );
      }
    },
    completion_state
  );
}

auto map_request_still_image_start(
  mln_map map, const mln_completion* completion
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!is_still_map_mode(live->map_mode)) {
    set_thread_error("map is not in static or tile mode");
    return MLN_STATUS_INVALID_STATE;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto control_lease = ControlLease{&live->control};
  auto submission = std::make_shared<ControlLease>(std::move(control_lease));
  auto submission_released = std::make_shared<std::atomic_bool>(false);
  auto release_submission = [submission,
                             submission_released]() noexcept -> void {
    if (!submission_released->exchange(true)) {
      submission->reset();
    }
  };
  auto completion_state = std::make_shared<Completion>(*completion);
  auto state = std::make_shared<OperationObject>(
    [completion_state](mln_status status, std::string diagnostic, std::any) {
      complete(completion_state, status, std::move(diagnostic));
    }
  );
  const auto submit_status = submit_runtime_operation(
    live->runtime_state, state,
    [map, live = std::move(live), state, release_submission]() mutable -> void {
      if (live->still_image_request_pending) {
        state->complete(
          MLN_STATUS_INVALID_STATE,
          "map already has a pending still-image request", {}
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
      } catch (...) {
        live->still_image_request_pending = false;
        live->still_image_operation.reset();
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    }
  );
  if (submit_status != MLN_STATUS_OK) {
    completion_state->reject();
  } else {
    completion_state->accept();
  }
  return submit_status;
}

// Render-facing helpers hold an attached-session child relationship, so
// asynchronous map close cannot retire their state.
auto map_scale_factor(mln_map map) -> double {
  const auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? default_scale_factor
                         : live->logical_extent.scale_factor;
}

// Returns worker-owned native state. Callers must already run on the runtime
// worker or use the posting helpers below.
auto map_native(MapObject* map) -> mln::Map* { return map->map.get(); }

namespace {

void discard_completion(void*, const mln_completion_result*) noexcept {}

auto discarded_completion() noexcept -> mln_completion {
  return mln_completion{
    .size = sizeof(mln_completion),
    .callback = discard_completion,
    .user_data = nullptr,
    .release_user_data = nullptr,
  };
}

}  // namespace

// Render-session resize enters the same ordered extent command as public
// resize, so one path owns logical extent and scale after map creation.
auto map_post_resize(mln_map map, mln_logical_extent extent) -> mln_status {
  const auto completion = discarded_completion();
  return map_resize(map, extent, &completion);
}

auto map_post_trigger_repaint(mln_map map) -> mln_status {
  const auto completion = discarded_completion();
  return map_request_repaint(map, &completion);
}

auto map_latest_update(mln_map map) -> std::shared_ptr<mln::UpdateParameters> {
  auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? nullptr : live->frontend->latest_update();
}
auto map_latest_update_generation(mln_map map) noexcept -> uint64_t {
  auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? 0 : live->frontend->latest_update_generation();
}
auto map_latest_update_snapshot(mln_map map, uint64_t& out_generation)
  -> std::shared_ptr<mln::UpdateParameters> {
  auto* live = handle_table<MapObject>().try_resolve(map);
  if (live == nullptr) {
    out_generation = 0;
    return nullptr;
  }
  return live->frontend->latest_update_snapshot(out_generation);
}

auto map_feature_state_snapshot(mln_map map)
  -> std::shared_ptr<const FeatureStateSnapshot> {
  auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? std::make_shared<FeatureStateSnapshot>()
                         : live->feature_state.snapshot();
}

auto map_set_render_session_publish_callback(
  mln_map map, std::function<void()> callback
) -> mln_status {
  const auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  live->frontend->set_session_publish_callback(std::move(callback));
  return MLN_STATUS_OK;
}

auto map_renderer_observer(mln_map map) -> mln::RendererObserver* {
  auto* live = handle_table<MapObject>().try_resolve(map);
  return live == nullptr ? nullptr : live->frontend->renderer_observer();
}

auto map_run_render_jobs(mln_map map) -> void {
  if (
    auto* live = handle_table<MapObject>().try_resolve(map); live != nullptr
  ) {
    live->frontend->run_render_jobs();
  }
}

auto map_quiesce_render_workers(mln_map map) -> void {
  auto* live = handle_table<MapObject>().try_resolve(map);
  if (live != nullptr) {
    live->frontend->wait_thread_pool();
  }
}

// The handle-table lock keeps the map live across the attached-session claim.
auto map_attach_render_target_session(mln_map map, void* session)
  -> mln_status {
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
  live->render_target_session = session;
  return MLN_STATUS_OK;
}

// The handle-table lock keeps the map live across the attached-session clear.
auto map_detach_render_target_session(mln_map map, void* session)
  -> mln_status {
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
  if (live->render_target_session != session) {
    set_thread_error("render session is not attached to this map");
    return MLN_STATUS_INVALID_STATE;
  }
  live->render_target_session = nullptr;
  return MLN_STATUS_OK;
}

auto map_set_style_url(mln_map map, const char* url) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (url == nullptr) {
    set_thread_error("url must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // Parse failures reported on this stack use runtime-worker state, so no
  // additional lock or queued diagnostic is needed.
  live->event_state->style_load_failed = false;
  live->event_state->style_load_failure.clear();
  live->map->getStyle().loadURL(url);
  if (live->event_state->style_load_failed) {
    set_thread_error(live->event_state->style_load_failure.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto map_set_style_json(mln_map map, mln_buffer_view json) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!validate_bytes(json, "style JSON")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  try {
    live->event_state->style_load_failed = false;
    live->event_state->style_load_failure.clear();
    live->map->getStyle().loadJSON(
      std::string{reinterpret_cast<const char*>(json.data), json.size}
    );
  } catch (const std::exception& exception) {
    // The diagnostic is this call's own status text, so it is set whatever the
    // mask selects; only the event is gated.
    set_thread_error(exception.what());
    if (
      event_selected(
        live->event_state->mask, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED
      )
    ) {
      push_runtime_map_event(
        live->runtime, map, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, 0,
        exception.what()
      );
    }
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (live->event_state->style_load_failed) {
    set_thread_error(live->event_state->style_load_failure.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}

auto start_map_string_operation(
  mln_map map, const mln_completion* completion,
  std::function<std::string(MapObject&)> read
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto submission = std::make_shared<ControlLease>(&live->control);
  auto completion_state = std::make_shared<Completion>(*completion);
  auto state =
    std::make_shared<OperationObject>([completion_state](
                                        mln_status status,
                                        std::string diagnostic, std::any result
                                      ) {
      if (status != MLN_STATUS_OK) {
        complete(completion_state, status, std::move(diagnostic));
        return;
      }
      auto* text = std::any_cast<std::string>(&result);
      if (text == nullptr) {
        complete(
          completion_state, MLN_STATUS_NATIVE_ERROR,
          "map string operation produced an invalid result"
        );
        return;
      }
      completion_state->resolve(
        [text = std::move(*text)](const mln_completion& descriptor) {
          const auto view =
            mln_buffer_view{.data = text.data(), .size = text.size()};
          invoke_completion(
            descriptor, MLN_STATUS_OK, MLN_COMMAND_DISPOSITION_COMMITTED, 0, {},
            &view, 1
          );
        }
      );
    });
  const auto submit_status = submit_runtime_operation(
    live->runtime_state, state,
    [live = std::move(live), state, read = std::move(read),
     submission = std::move(submission)]() mutable -> void {
      try {
        state->complete(MLN_STATUS_OK, {}, std::any{read(*live)});
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    }
  );
  if (submit_status == MLN_STATUS_OK)
    completion_state->accept();
  else
    completion_state->reject();
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
  mln_map map, const mln_feature_state_selector* selector, mln_buffer_view state
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, true);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }

  auto native_state = to_native_json_value(state);
  if (!native_state) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto* state_object = native_state->getObject();
  if (state_object == nullptr) {
    set_thread_error("feature state value must be a JSON object");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  live->feature_state.set(
    feature_state_string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    feature_state_string_from_view(selector->feature_id), *state_object
  );
  live->map->triggerRepaint();
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
  return start_map_string_operation(
    map, completion,
    [source_id = std::move(source_id), source_layer = std::move(source_layer),
     feature_id = std::move(feature_id)](MapObject& live) -> std::string {
      auto state = live.feature_state.get(source_id, source_layer, feature_id);
      return serialize_json_value(mln::Value{std::move(state)});
    }
  );
}

auto map_remove_feature_state(
  mln_map map, const mln_feature_state_selector* selector
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto selector_status = validate_feature_state_selector(selector, false);
  if (selector_status != MLN_STATUS_OK) {
    return selector_status;
  }

  live->feature_state.remove(
    feature_state_string_from_view(selector->source_id),
    feature_state_source_layer(*selector),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, selector->feature_id
    ),
    optional_selector_string(
      *selector, MLN_FEATURE_STATE_SELECTOR_STATE_KEY, selector->state_key
    )
  );
  live->map->triggerRepaint();
  return MLN_STATUS_OK;
}

auto map_loaded_style_json_start(mln_map map, const mln_completion* completion)
  -> mln_status {
  return start_map_string_operation(
    map, completion, [](MapObject& live) -> std::string {
      return live.map->getStyle().getJSON();
    }
  );
}

auto map_style_url_start(mln_map map, const mln_completion* completion)
  -> mln_status {
  return start_map_string_operation(
    map, completion,
    [](MapObject& live) -> std::string { return live.map->getStyle().getURL(); }
  );
}

auto map_set_event_mask(
  mln_map map, uint64_t mask, const mln_completion* completion
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (
    completion_status != MLN_STATUS_OK ||
    (mask & ~static_cast<uint64_t>(MLN_RUNTIME_EVENT_MASK_ALL)) != 0U
  ) {
    if (completion_status == MLN_STATUS_OK)
      set_thread_error("mask must be valid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto submission = std::make_shared<ControlLease>(&live->control);
  auto completion_state = std::make_shared<Completion>(*completion);
  return submit_runtime_command(
    live->runtime_state,
    [live, mask, submission = std::move(submission),
     completion_state](uint64_t) mutable -> void {
      try {
        live->event_state->mask.store(mask, std::memory_order_relaxed);
        const auto generation = publish_map_snapshot(*live);
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK,
          generation
        );
      } catch (...) {
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_FAILED,
          MLN_STATUS_NATIVE_ERROR, 0,
          exception_message(std::current_exception())
        );
      }
    },
    completion_state
  );
}

auto map_camera_snapshot_get(
  mln_map map, mln_camera_options* out_camera, uint64_t* out_generation
) -> mln_status {
  if (
    out_camera == nullptr || out_camera->size < sizeof(mln_camera_options) ||
    out_generation == nullptr
  ) {
    set_thread_error("camera output and generation must be valid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto snapshot = mln_map_snapshot{};
  snapshot.size = sizeof(mln_map_snapshot);
  const auto status = map_snapshot_get(map, &snapshot);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  *out_camera = snapshot.camera;
  *out_generation = snapshot.generation;
  return MLN_STATUS_OK;
}

namespace {

template <typename Mutation>
auto submit_camera_command(
  mln_map map, Mutation mutation, const mln_completion* completion
) -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) return MLN_STATUS_INVALID_ARGUMENT;
  if (!live->control.acquire()) return MLN_STATUS_INVALID_STATE;
  auto submission = std::make_shared<ControlLease>(&live->control);
  auto completion_state = std::make_shared<Completion>(*completion);
  return submit_runtime_command(
    live->runtime_state,
    [map, live, mutation = std::move(mutation),
     submission = std::move(submission),
     completion_state](uint64_t) mutable -> void {
      try {
        mutation(*live, map);
        const auto generation = publish_map_snapshot(*live);
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK,
          generation
        );
      } catch (...) {
        complete_command(
          completion_state, MLN_COMMAND_DISPOSITION_FAILED,
          MLN_STATUS_NATIVE_ERROR, 0,
          exception_message(std::current_exception())
        );
      }
    },
    completion_state
  );
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
  const auto camera_status = validate_camera_options(&update->camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  const auto animation_status = validate_animation_options(&update->animation);
  if (animation_status != MLN_STATUS_OK) {
    return animation_status;
  }
  const auto copied = *update;
  return submit_camera_command(
    map,
    [copied](MapObject& live, mln_map map_handle) -> void {
      if (
        copied.gesture_phase == MLN_GESTURE_PHASE_BEGIN ||
        copied.gesture_phase == MLN_GESTURE_PHASE_UPDATE
      ) {
        live.map->setGestureInProgress(true);
      }
      switch (copied.mode) {
        case MLN_CAMERA_UPDATE_MODE_JUMP:
          live.map->jumpTo(to_native_camera(copied.camera));
          break;
        case MLN_CAMERA_UPDATE_MODE_EASE:
          live.map->easeTo(
            to_native_camera(copied.camera),
            to_native_animation(
              live.runtime, map_handle, live.event_state, &copied.animation
            )
          );
          break;
        case MLN_CAMERA_UPDATE_MODE_FLY:
          live.map->flyTo(
            to_native_camera(copied.camera),
            to_native_animation(
              live.runtime, map_handle, live.event_state, &copied.animation
            )
          );
          break;
        default:
          break;
      }
      if (copied.gesture_phase == MLN_GESTURE_PHASE_CANCEL) {
        live.map->cancelTransitions();
      }
      if (
        copied.gesture_phase == MLN_GESTURE_PHASE_END ||
        copied.gesture_phase == MLN_GESTURE_PHASE_CANCEL
      ) {
        live.map->setGestureInProgress(false);
      }
    },
    completion
  );
}

auto map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion
) -> mln_status {
  if (delta == nullptr || delta->size < sizeof(mln_camera_delta)) {
    set_thread_error("camera delta must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (delta->kind > MLN_CAMERA_DELTA_PITCH) {
    set_thread_error("camera delta kind is invalid");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    delta->kind == MLN_CAMERA_DELTA_MOVE &&
    validate_screen_point(delta->offset) != MLN_STATUS_OK
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    delta->kind == MLN_CAMERA_DELTA_SCALE &&
    (!std::isfinite(delta->amount) || delta->amount <= 0)
  ) {
    set_thread_error("camera scale must be finite and positive");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (delta->kind == MLN_CAMERA_DELTA_BEARING ||
     delta->kind == MLN_CAMERA_DELTA_PITCH) &&
    !std::isfinite(delta->amount)
  ) {
    set_thread_error("camera angle delta must be finite");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    delta->has_anchor && delta->kind != MLN_CAMERA_DELTA_SCALE &&
    delta->kind != MLN_CAMERA_DELTA_BEARING
  ) {
    set_thread_error("only scale and bearing camera deltas accept an anchor");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    delta->has_anchor && validate_screen_point(delta->anchor) != MLN_STATUS_OK
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (validate_animation_options(&delta->animation) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto copied = *delta;
  return submit_camera_command(
    map,
    [copied](MapObject& live, mln_map map_handle) -> void {
      const auto animation = to_native_animation(
        live.runtime, map_handle, live.event_state, &copied.animation
      );
      const auto anchor =
        copied.has_anchor
          ? std::optional<mln::ScreenCoordinate>{to_native_screen_point(
              copied.anchor
            )}
          : std::nullopt;
      switch (copied.kind) {
        case MLN_CAMERA_DELTA_MOVE:
          live.map->moveBy(to_native_screen_point(copied.offset), animation);
          break;
        case MLN_CAMERA_DELTA_SCALE:
          live.map->scaleBy(copied.amount, anchor, animation);
          break;
        case MLN_CAMERA_DELTA_BEARING: {
          const auto current = live.map->getCameraOptions();
          auto camera = mln::CameraOptions{}.withBearing(
            current.bearing.value_or(0) + copied.amount
          );
          if (anchor.has_value()) camera.withAnchor(*anchor);
          live.map->easeTo(camera, animation);
          break;
        }
        case MLN_CAMERA_DELTA_PITCH:
          live.map->pitchBy(-copied.amount, animation);
          break;
        default:
          break;
      }
    },
    completion
  );
}

auto map_camera_query_start(mln_map map, const mln_completion* completion)
  -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;
  auto live = handle_table<MapObject>().lease(map);
  if (live == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!live->control.acquire()) {
    return MLN_STATUS_INVALID_STATE;
  }
  auto submission = std::make_shared<ControlLease>(&live->control);
  auto completion_state = std::make_shared<Completion>(*completion);
  auto state =
    std::make_shared<OperationObject>([completion_state](
                                        mln_status status,
                                        std::string diagnostic, std::any result
                                      ) {
      auto* value = std::any_cast<mln_camera_query_result>(&result);
      if (status != MLN_STATUS_OK || value == nullptr) {
        complete(
          completion_state,
          status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
          status == MLN_STATUS_OK ? "camera query produced an invalid result"
                                  : std::move(diagnostic)
        );
        return;
      }
      complete_value(completion_state, MLN_STATUS_OK, {}, *value);
    });
  const auto submit_status = submit_runtime_operation(
    live->runtime_state, state,
    [live = std::move(live), state,
     submission = std::move(submission)]() mutable -> void {
      try {
        const auto generation = publish_map_snapshot(*live);
        state->complete(
          MLN_STATUS_OK, {},
          std::any{mln_camera_query_result{
            .size = sizeof(mln_camera_query_result),
            .reserved = 0,
            .generation = generation,
            .camera = from_native_camera(live->map->getCameraOptions())
          }}
        );
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    }
  );
  if (submit_status == MLN_STATUS_OK)
    completion_state->accept();
  else
    completion_state->reject();
  return submit_status;
}

auto map_set_debug_options(mln_map map, uint32_t options) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_debug_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  live->map->setDebug(to_native_debug_options(options));
  return MLN_STATUS_OK;
}

auto map_set_rendering_stats_view_enabled(mln_map map, bool enabled)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->map->enableRenderingStatsView(enabled);
  return MLN_STATUS_OK;
}

auto map_dump_debug_logs(mln_map map) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  live->map->dumpDebugLogs();
  return MLN_STATUS_OK;
}

auto map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_viewport_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0U) {
    live->map->setNorthOrientation(
      to_native_north_orientation(options->north_orientation)
    );
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0U) {
    live->map->setConstrainMode(
      to_native_constrain_mode(options->constrain_mode)
    );
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0U) {
    live->map->setViewportMode(to_native_viewport_mode(options->viewport_mode));
  }
  if ((options->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0U) {
    live->map->setFrustumOffset(to_native_edge_insets(options->frustum_offset));
  }
  return MLN_STATUS_OK;
}

auto map_set_tile_options(mln_map map, const mln_map_tile_options* options)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_tile_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  if ((options->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0U) {
    live->map->setPrefetchZoomDelta(
      static_cast<uint8_t>(options->prefetch_zoom_delta)
    );
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0U) {
    live->map->setTileLodMinRadius(options->lod_min_radius);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0U) {
    live->map->setTileLodScale(options->lod_scale);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0U) {
    live->map->setTileLodPitchThreshold(options->lod_pitch_threshold);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0U) {
    live->map->setTileLodZoomShift(options->lod_zoom_shift);
  }
  if ((options->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0U) {
    live->map->setTileLodMode(to_native_tile_lod_mode(options->lod_mode));
  }
  return MLN_STATUS_OK;
}

auto map_pixel_for_lat_lng(
  mln_map map, mln_lat_lng coordinate, mln_screen_point* out_point
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_point == nullptr) {
    set_thread_error("out_point must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinate_status = validate_lat_lng(coordinate);
  if (coordinate_status != MLN_STATUS_OK) {
    return coordinate_status;
  }

  *out_point = from_native_screen_point(
    live->map->pixelForLatLng(to_native_lat_lng(coordinate))
  );
  return MLN_STATUS_OK;
}

auto map_lat_lng_for_pixel(
  mln_map map, mln_screen_point point, mln_lat_lng* out_coordinate,
  mln::LatLng::WrapMode wrap_mode
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_coordinate == nullptr) {
    set_thread_error("out_coordinate must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto point_status = validate_screen_point(point);
  if (point_status != MLN_STATUS_OK) {
    return point_status;
  }

  *out_coordinate = from_native_lat_lng(
    live->map->latLngForPixel(to_native_screen_point(point), wrap_mode)
  );
  return MLN_STATUS_OK;
}

auto map_pixels_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  mln_screen_point* out_points
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (coordinate_count != 0 && out_points == nullptr) {
    set_thread_error(
      "out_points must not be null when coordinate_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinates_status =
    validate_lat_lng_array(coordinates, coordinate_count, true);
  if (coordinates_status != MLN_STATUS_OK) {
    return coordinates_status;
  }
  if (coordinate_count == 0) {
    return MLN_STATUS_OK;
  }

  const auto native_coordinates =
    to_native_lat_lngs(coordinates, coordinate_count);
  const auto pixels = live->map->pixelsForLatLngs(native_coordinates);
  auto output = std::span<mln_screen_point>{out_points, pixels.size()};
  auto output_position = output.begin();
  for (const auto& pixel : pixels) {
    *output_position = from_native_screen_point(pixel);
    ++output_position;
  }
  return MLN_STATUS_OK;
}

auto map_lat_lngs_for_pixels(
  mln_map map, const mln_screen_point* points, size_t point_count,
  mln_lat_lng* out_coordinates, mln::LatLng::WrapMode wrap_mode
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (point_count != 0 && out_coordinates == nullptr) {
    set_thread_error(
      "out_coordinates must not be null when point_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto points_status = validate_screen_point_array(points, point_count);
  if (points_status != MLN_STATUS_OK) {
    return points_status;
  }
  if (point_count == 0) {
    return MLN_STATUS_OK;
  }

  const auto native_points = to_native_screen_points(points, point_count);
  const auto coordinates =
    live->map->latLngsForPixels(native_points, wrap_mode);
  auto output = std::span<mln_lat_lng>{out_coordinates, coordinates.size()};
  auto output_position = output.begin();
  for (const auto& coordinate : coordinates) {
    *output_position = from_native_lat_lng(coordinate);
    ++output_position;
  }
  return MLN_STATUS_OK;
}

auto map_pixel_for_lat_lng_start(
  mln_map map, mln_lat_lng coordinate, const mln_completion* completion
) -> mln_status {
  if (validate_lat_lng(coordinate) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return start_geometry_operation(
    map, GeometryOperationKind::PixelForCoordinate,
    [map, coordinate](GeometryOperationResult& result) {
      return map_pixel_for_lat_lng(map, coordinate, &result.point);
    },
    completion
  );
}

auto start_coordinate_for_pixel(
  mln_map map, mln_screen_point point, bool unwrapped,
  const mln_completion* completion
) -> mln_status {
  if (validate_screen_point(point) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto wrap_mode =
    unwrapped ? mln::LatLng::Unwrapped : mln::LatLng::Wrapped;
  return start_geometry_operation(
    map, GeometryOperationKind::CoordinateForPixel,
    [map, point, wrap_mode](GeometryOperationResult& result) {
      return map_lat_lng_for_pixel(map, point, &result.coordinate, wrap_mode);
    },
    completion
  );
}

auto map_lat_lng_for_pixel_start(
  mln_map map, mln_screen_point point, const mln_completion* completion
) -> mln_status {
  return start_coordinate_for_pixel(map, point, false, completion);
}

auto map_lat_lng_for_pixel_unwrapped_start(
  mln_map map, mln_screen_point point, const mln_completion* completion
) -> mln_status {
  return start_coordinate_for_pixel(map, point, true, completion);
}

auto map_pixels_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_completion* completion
) -> mln_status {
  if (
    validate_lat_lng_array(coordinates, coordinate_count, true) != MLN_STATUS_OK
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto copied = std::vector<mln_lat_lng>{};
  if (coordinate_count != 0) {
    copied.assign(coordinates, coordinates + coordinate_count);
  }
  return start_geometry_operation(
    map, GeometryOperationKind::PixelsForCoordinates,
    [map, copied = std::move(copied)](GeometryOperationResult& result) {
      result.points.resize(copied.size());
      return map_pixels_for_lat_lngs(
        map, copied.data(), copied.size(), result.points.data()
      );
    },
    completion
  );
}

auto start_coordinates_for_pixels(
  mln_map map, const mln_screen_point* points, size_t point_count,
  bool unwrapped, const mln_completion* completion
) -> mln_status {
  if (validate_screen_point_array(points, point_count) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto copied = std::vector<mln_screen_point>{};
  if (point_count != 0) {
    copied.assign(points, points + point_count);
  }
  const auto wrap_mode =
    unwrapped ? mln::LatLng::Unwrapped : mln::LatLng::Wrapped;
  return start_geometry_operation(
    map, GeometryOperationKind::CoordinatesForPixels,
    [map, copied = std::move(copied),
     wrap_mode](GeometryOperationResult& result) {
      result.coordinates.resize(copied.size());
      return map_lat_lngs_for_pixels(
        map, copied.data(), copied.size(), result.coordinates.data(), wrap_mode
      );
    },
    completion
  );
}

auto map_lat_lngs_for_pixels_start(
  mln_map map, const mln_screen_point* points, size_t point_count,
  const mln_completion* completion
) -> mln_status {
  return start_coordinates_for_pixels(
    map, points, point_count, false, completion
  );
}

auto map_lat_lngs_for_pixels_unwrapped_start(
  mln_map map, const mln_screen_point* points, size_t point_count,
  const mln_completion* completion
) -> mln_status {
  return start_coordinates_for_pixels(
    map, points, point_count, true, completion
  );
}

auto map_projection_create_start(mln_map map, const mln_completion* completion)
  -> mln_status {
  const auto completion_status = validate_completion(completion);
  if (completion_status != MLN_STATUS_OK) return completion_status;

  auto& table = handle_table<MapObject>();
  const std::scoped_lock lock(table.mutex());
  auto parent = table.lease_locked(map);
  if (parent == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto runtime = lease_runtime(parent->runtime);
  if (runtime == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto completion_state = std::make_shared<Completion>(*completion);
  auto state = std::make_shared<OperationObject>([completion_state](
                                                   mln_status status,
                                                   std::string diagnostic,
                                                   std::any result
                                                 ) {
    auto* projection =
      std::any_cast<std::shared_ptr<MapProjectionObject>>(&result);
    if (
      status != MLN_STATUS_OK || projection == nullptr || *projection == nullptr
    ) {
      complete(
        completion_state,
        status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
        status == MLN_STATUS_OK
          ? "projection creation produced an invalid result"
          : std::move(diagnostic)
      );
      return;
    }
    const auto handle = handle_table<MapProjectionObject>().insert(*projection);
    complete_value(completion_state, MLN_STATUS_OK, {}, handle);
  });
  const auto submit_status =
    submit_runtime_operation(runtime, state, [parent, state]() mutable -> void {
      try {
        auto projection = std::make_shared<MapProjectionObject>();
        projection->projection =
          std::make_unique<mln::MapProjection>(*parent->map);
        state->complete(MLN_STATUS_OK, {}, std::any{std::move(projection)});
      } catch (...) {
        state->complete(
          MLN_STATUS_NATIVE_ERROR, exception_message(std::current_exception()),
          {}
        );
      }
    });
  if (submit_status == MLN_STATUS_OK)
    completion_state->accept();
  else
    completion_state->reject();
  return submit_status;
}

auto map_projection_close(mln_map_projection projection) -> mln_status {
  auto& table = handle_table<MapProjectionObject>();
  std::shared_ptr<MapProjectionObject> owned;
  {
    const std::scoped_lock lock(table.mutex());
    owned = table.lease_locked(projection);
    if (owned == nullptr) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    static_cast<void>(table.remove_locked(projection));
  }
  {
    // Waits for projection calls already running on other threads, then
    // destroys the projection. A racing call that leased the handle before the
    // removal observes the null projection and reports a stale handle.
    const std::scoped_lock call_lock(owned->call_mutex);
    owned->projection.reset();
  }
  return MLN_STATUS_OK;
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

auto map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding
) -> mln_status {
  const auto coordinates_status =
    validate_lat_lng_array(coordinates, coordinate_count, false);
  if (coordinates_status != MLN_STATUS_OK) {
    return coordinates_status;
  }
  const auto padding_status = validate_edge_insets(padding);
  if (padding_status != MLN_STATUS_OK) {
    return padding_status;
  }
  const auto native_coordinates =
    to_native_lat_lngs(coordinates, coordinate_count);
  const auto native_padding = to_native_edge_insets(padding);
  return with_projection(
    projection,
    [&native_coordinates, &native_padding](mln::MapProjection& value) {
      value.setVisibleCoordinates(native_coordinates, native_padding);
    }
  );
}

auto map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding
) -> mln_status {
  const auto padding_status = validate_edge_insets(padding);
  if (padding_status != MLN_STATUS_OK) {
    return padding_status;
  }
  const auto native_geometry = to_native_geometry(geometry);
  if (!native_geometry) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto coordinates = geometry_lat_lngs(*native_geometry);
  if (coordinates.empty()) {
    set_thread_error("geometry must contain at least one coordinate");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto native_padding = to_native_edge_insets(padding);
  return with_projection(
    projection, [&coordinates, &native_padding](mln::MapProjection& value) {
      value.setVisibleCoordinates(coordinates, native_padding);
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
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
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

  *out_camera = from_native_camera(live->map->cameraForLatLngBounds(
    to_native_lat_lng_bounds(bounds), camera_fit_padding(fit_options),
    camera_fit_bearing(fit_options), camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_camera_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
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

  *out_camera = from_native_camera(live->map->cameraForLatLngs(
    to_native_lat_lngs(coordinates, coordinate_count),
    camera_fit_padding(fit_options), camera_fit_bearing(fit_options),
    camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_camera_options* out_camera
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
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

  *out_camera = from_native_camera(live->map->cameraForGeometry(
    *native_geometry, camera_fit_padding(fit_options),
    camera_fit_bearing(fit_options), camera_fit_pitch(fit_options)
  ));
  return MLN_STATUS_OK;
}

auto map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera, mln_lat_lng_bounds* out_bounds
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  if (out_bounds == nullptr) {
    set_thread_error("out_bounds must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_bounds = from_native_lat_lng_bounds(
    live->map->latLngBoundsForCamera(to_native_camera(*camera))
  );
  return MLN_STATUS_OK;
}

auto map_lat_lng_bounds_for_camera_unwrapped(
  mln_map map, const mln_camera_options* camera, mln_lat_lng_bounds* out_bounds
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto camera_status = validate_camera_options(camera);
  if (camera_status != MLN_STATUS_OK) {
    return camera_status;
  }
  if (out_bounds == nullptr) {
    set_thread_error("out_bounds must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  *out_bounds = from_native_lat_lng_bounds(
    live->map->latLngBoundsForCameraUnwrapped(to_native_camera(*camera))
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
  return start_geometry_operation(
    map, GeometryOperationKind::CameraForBounds,
    [map, bounds, fit, has_fit](GeometryOperationResult& result) {
      result.camera = camera_options_default();
      return map_camera_for_lat_lng_bounds(
        map, bounds, has_fit ? &fit : nullptr, &result.camera
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
  return start_geometry_operation(
    map, GeometryOperationKind::CameraForCoordinates,
    [map, copied = std::move(copied), fit,
     has_fit](GeometryOperationResult& result) {
      result.camera = camera_options_default();
      return map_camera_for_lat_lngs(
        map, copied.data(), copied.size(), has_fit ? &fit : nullptr,
        &result.camera
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
  return start_geometry_operation(
    map, GeometryOperationKind::CameraForGeometry,
    [map, bytes = std::move(bytes), fit,
     has_fit](GeometryOperationResult& result) -> mln_status {
      result.camera = camera_options_default();
      const auto view =
        mln_buffer_view{.data = bytes.data(), .size = bytes.size()};
      return map_camera_for_geometry(
        map, view, has_fit ? &fit : nullptr, &result.camera
      );
    },
    completion
  );
}

auto start_bounds_for_camera(
  mln_map map, const mln_camera_options* camera, bool unwrapped,
  const mln_completion* completion
) -> mln_status {
  if (validate_camera_options(camera) != MLN_STATUS_OK) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto copied = *camera;
  const auto kind = unwrapped ? GeometryOperationKind::UnwrappedBoundsForCamera
                              : GeometryOperationKind::BoundsForCamera;
  return start_geometry_operation(
    map, kind,
    [map, copied, unwrapped](GeometryOperationResult& result) {
      return unwrapped
               ? map_lat_lng_bounds_for_camera_unwrapped(
                   map, &copied, &result.bounds
                 )
               : map_lat_lng_bounds_for_camera(map, &copied, &result.bounds);
    },
    completion
  );
}

auto map_lat_lng_bounds_for_camera_start(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) -> mln_status {
  return start_bounds_for_camera(map, camera, false, completion);
}

auto map_lat_lng_bounds_for_camera_unwrapped_start(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) -> mln_status {
  return start_bounds_for_camera(map, camera, true, completion);
}

auto map_set_bounds(mln_map map, const mln_bound_options* options)
  -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_bound_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  // Native setBounds only applies optionals that are set, so this preserves
  // constraints omitted from options->fields.
  live->map->setBounds(to_native_bound_options(*options));
  return MLN_STATUS_OK;
}

auto map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options
) -> mln_status {
  MapObject* live = nullptr;
  const auto status = validate_map(map, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  const auto options_status = validate_free_camera_options(options);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }

  live->map->setFreeCameraOptions(to_native_free_camera(*options));
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
    [map, copied = std::move(copied)]() mutable -> mln_status {
      MapObject* live = nullptr;
      const auto status = validate_map(map, live);
      if (status != MLN_STATUS_OK) {
        return status;
      }
      live->map->setProjectionMode(std::move(copied));
      return MLN_STATUS_OK;
    },
    completion
  );
}

}  // namespace mln::core
