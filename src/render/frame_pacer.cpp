#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "render/frame_pacer.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core {

namespace {

auto frame_pacer_table() -> HandleTable<FramePacerObject>& {
  return handle_table<FramePacerObject>();
}

// A vsync can arrive a little before a whole display period has passed, so a
// limit equal to the display's rate would skip every other vsync if the pacer
// waited the full period. It accepts a vsync this early instead.
constexpr auto early_vsync_percent = std::int64_t{10};

}  // namespace

FramePacer::FramePacer(std::chrono::nanoseconds minimum_interval) noexcept
    : minimum_interval_{minimum_interval} {}

auto FramePacer::add_session() -> void {
  const auto lock = std::scoped_lock{mutex_};
  waiting_.reserve(sessions_ + 1);
  ++sessions_;
}

auto FramePacer::remove_session(mln_render_session session) noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  std::erase(waiting_, session);
  --sessions_;
}

auto FramePacer::request_frame(mln_render_session session) noexcept -> void {
  {
    const auto lock = std::scoped_lock{mutex_};
    if (std::ranges::find(waiting_, session) != waiting_.end()) return;
    // add_session() reserved the room, so this doesn't allocate.
    waiting_.push_back(session);
    if (vsync_scheduled_) return;
    vsync_scheduled_ = true;
  }
  schedule_vsync();
}

auto FramePacer::too_early_locked(std::chrono::nanoseconds time) const -> bool {
  // A clock that moves backward restarts the limit.
  return last_demand_time_.has_value() && time >= *last_demand_time_ &&
         time - *last_demand_time_ < minimum_interval_;
}

auto FramePacer::on_vsync(std::chrono::nanoseconds time) noexcept -> void {
  auto due = std::vector<mln_render_session>{};
  auto skip = false;
  {
    const auto lock = std::scoped_lock{mutex_};
    if (waiting_.empty()) {
      vsync_scheduled_ = false;
      return;
    }
    skip = too_early_locked(time);
    if (!skip) {
      try {
        due = waiting_;
      } catch (const std::bad_alloc&) {
        // Tries again at the next vsync.
        skip = true;
      }
    }
    if (!skip) {
      waiting_.clear();
      last_demand_time_ = time;
      vsync_scheduled_ = false;
    }
  }
  if (skip) {
    schedule_vsync();
    return;
  }
  constexpr auto demand = mln_frame_demand{
    .size = sizeof(mln_frame_demand),
    .flags = 0,
    .token = 0,
  };
  for (const auto session : due) {
    // A session that has gone or failed since it asked refuses the demand,
    // and a demand that can't allocate leaves the session as it was.
    try {
      static_cast<void>(render_session_demand_frame(session, &demand));
    } catch (const std::exception&) {
    }
  }
}

auto frame_pacer_minimum_interval(
  const mln_frame_pacer_options* options,
  std::chrono::nanoseconds& out_minimum_interval
) -> mln_status {
  out_minimum_interval = std::chrono::nanoseconds::zero();
  if (options == nullptr) return MLN_STATUS_OK;
  if (options->size < sizeof(mln_frame_pacer_options)) {
    set_thread_error("mln_frame_pacer_options.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & ~MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND) != 0U
  ) {
    set_thread_error("mln_frame_pacer_options.fields contains unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    (options->fields & MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND) == 0U
  )
    return MLN_STATUS_OK;
  if (options->maximum_frames_per_second == 0) {
    set_thread_error(
      "mln_frame_pacer_options.maximum_frames_per_second must be greater than "
      "zero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto period = std::chrono::nanoseconds{std::chrono::seconds{1}} /
                      options->maximum_frames_per_second;
  out_minimum_interval = period * (100 - early_vsync_percent) / 100;
  return MLN_STATUS_OK;
}

auto validate_frame_pacer_output(const mln_frame_pacer* out_pacer)
  -> mln_status {
  if (out_pacer == nullptr || *out_pacer != MLN_HANDLE_NULL) {
    set_thread_error("out_pacer must point to a null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto frame_pacer_insert(
  std::shared_ptr<FramePacer> pacer, mln_frame_pacer* out_pacer
) -> mln_status {
  *out_pacer = frame_pacer_table().insert(
    std::make_shared<FramePacerObject>(
      FramePacerObject{.pacer = std::move(pacer)}
    )
  );
  return MLN_STATUS_OK;
}

auto lease_frame_pacer(mln_frame_pacer pacer) -> std::shared_ptr<FramePacer> {
  const auto object = frame_pacer_table().lease(pacer);
  return object == nullptr ? nullptr : object->pacer;
}

auto frame_pacer_release(mln_frame_pacer pacer) noexcept -> void {
  if (pacer == MLN_HANDLE_NULL) return;
  // The sessions that use the pacer keep it running.
  static_cast<void>(frame_pacer_table().remove(pacer));
}

}  // namespace mln::core
