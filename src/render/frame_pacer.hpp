#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// The part of a frame pacer that every platform shares: the render sessions
// that wait for the next vsync, and the frame rate limit.
//
// A platform pacer derives from it, implements schedule_vsync() with the
// platform's vsync source, and calls on_vsync() from that source's callback.
// The pacer calls schedule_vsync() again only after on_vsync() runs.
//
// Lock order: a session calls request_frame() under its control_mutex, which
// takes the pacer's lock. on_vsync() releases the pacer's lock before it
// demands frames, so no session lock is ever taken under the pacer's.
class FramePacer {
 public:
  // `minimum_interval` is the shortest time between two vsyncs at which the
  // pacer demands frames, or zero for no limit.
  explicit FramePacer(std::chrono::nanoseconds minimum_interval) noexcept;
  FramePacer(const FramePacer&) = delete;
  FramePacer(FramePacer&&) = delete;
  auto operator=(const FramePacer&) -> FramePacer& = delete;
  auto operator=(FramePacer&&) -> FramePacer& = delete;
  virtual ~FramePacer() = default;

  // Makes room for one more session's request, so that request_frame() needs
  // no allocation. Throws std::bad_alloc.
  auto add_session() -> void;
  // Gives back the room that add_session() made, and drops any request of
  // `session` that waits for a vsync.
  auto remove_session(mln_render_session session) noexcept -> void;
  // Asks for a frame demand for `session` at the next vsync that the limit
  // allows. A session asks only after add_session(), and asks once until the
  // demand arrives.
  auto request_frame(mln_render_session session) noexcept -> void;

 protected:
  // Arranges one call to on_vsync() at the display's next vsync. Runs on any
  // thread, possibly under a session's control_mutex, so it must not block or
  // call into a session.
  virtual auto schedule_vsync() noexcept -> void = 0;
  // Called by the platform at the vsync that schedule_vsync() asked for, with
  // the vsync's time on a monotonic clock, and without any library lock held.
  // Demands a frame without flags for each session that asked since the last
  // demands, unless the limit skips this vsync and waits for the next.
  auto on_vsync(std::chrono::nanoseconds time) noexcept -> void;

 private:
  [[nodiscard]] auto too_early_locked(std::chrono::nanoseconds time) const
    -> bool;

  const std::chrono::nanoseconds minimum_interval_;
  std::mutex mutex_;
  std::size_t sessions_ = 0;
  // Reserved for every session that add_session() counted.
  std::vector<mln_render_session> waiting_;
  bool vsync_scheduled_ = false;
  std::optional<std::chrono::nanoseconds> last_demand_time_;
};

// Validates `options`, which may be null, and returns the minimum interval
// that its frame rate limit gives FramePacer.
auto frame_pacer_minimum_interval(
  const mln_frame_pacer_options* options,
  std::chrono::nanoseconds& out_minimum_interval
) -> mln_status;

// Checks that `out_pacer` can take a new handle, for a platform's create
// function to call before it creates anything.
auto validate_frame_pacer_output(const mln_frame_pacer* out_pacer)
  -> mln_status;

// Publishes a platform pacer as a handle.
auto frame_pacer_insert(
  std::shared_ptr<FramePacer> pacer, mln_frame_pacer* out_pacer
) -> mln_status;

// The pacer that `pacer` names, or null after recording a handle fault.
[[nodiscard]] auto lease_frame_pacer(mln_frame_pacer pacer)
  -> std::shared_ptr<FramePacer>;

auto frame_pacer_release(mln_frame_pacer pacer) noexcept -> void;

struct FramePacerObject {
  std::shared_ptr<FramePacer> pacer;
};

template <>
struct HandleTraits<FramePacerObject> {
  static constexpr auto kind = HandleKind::FramePacer;
  static constexpr auto leasable = true;
};

}  // namespace mln::core
