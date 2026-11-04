// A frame pacer on an OH_NativeVSync of its own.
//
// The native VSync calls back on a thread that it creates, so a request can
// come from any thread and needs no event loop of the host's. A request can't
// be cancelled, and destroying the native VSync while its callback runs would
// free what the callback uses, so each request holds the VSync side of the
// pacer until its callback has run. When that callback lets go of it last, the
// native VSync is destroyed on its own thread, which its event runner allows.

#include <chrono>
#include <memory>
#include <mutex>
#include <string_view>
#include <utility>

#include <native_vsync/native_vsync.h>

#include "platform/ohos/frame_pacer.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/frame_pacer.hpp"

namespace mln::platform::ohos {

namespace {

class NativeVSyncPacer;

// What the native VSync's callback shares with the pacer. A waiting request
// holds it until its callback runs, so it can outlive the pacer.
struct VSyncSide final : std::enable_shared_from_this<VSyncSide> {
  explicit VSyncSide(OH_NativeVSync* vsync_) noexcept : vsync{vsync_} {}
  VSyncSide(const VSyncSide&) = delete;
  VSyncSide(VSyncSide&&) = delete;
  auto operator=(const VSyncSide&) -> VSyncSide& = delete;
  auto operator=(VSyncSide&&) -> VSyncSide& = delete;
  ~VSyncSide() { OH_NativeVSync_Destroy(vsync); }

  OH_NativeVSync* const vsync;
  // Set before the first request.
  std::weak_ptr<NativeVSyncPacer> pacer;
  std::mutex mutex;
  // Set while a request waits for its callback.
  std::shared_ptr<VSyncSide> request_hold;
};

class NativeVSyncPacer final : public core::FramePacer {
 public:
  NativeVSyncPacer(
    std::chrono::nanoseconds minimum_interval, std::shared_ptr<VSyncSide> side
  ) noexcept
      : FramePacer{minimum_interval}, side_{std::move(side)} {}
  NativeVSyncPacer(const NativeVSyncPacer&) = delete;
  NativeVSyncPacer(NativeVSyncPacer&&) = delete;
  auto operator=(const NativeVSyncPacer&) -> NativeVSyncPacer& = delete;
  auto operator=(NativeVSyncPacer&&) -> NativeVSyncPacer& = delete;
  ~NativeVSyncPacer() override = default;

  // Runs on the native VSync's thread at a vsync.
  auto vsync(std::chrono::nanoseconds time) noexcept -> void { on_vsync(time); }

 protected:
  auto schedule_vsync() noexcept -> void override;

 private:
  std::shared_ptr<VSyncSide> side_;
};

// The timestamp is in nanoseconds on CLOCK_MONOTONIC.
void on_frame(long long timestamp, void* data) {
  auto& side = *static_cast<VSyncSide*>(data);
  // Declared first, so it lets go of the VSync side last.
  auto hold = std::shared_ptr<VSyncSide>{};
  {
    const auto lock = std::scoped_lock{side.mutex};
    hold = std::move(side.request_hold);
  }
  if (const auto pacer = side.pacer.lock()) {
    pacer->vsync(std::chrono::nanoseconds{timestamp});
  }
}

auto NativeVSyncPacer::schedule_vsync() noexcept -> void {
  {
    const auto lock = std::scoped_lock{side_->mutex};
    side_->request_hold = side_;
  }
  // The callback can run before this returns. A request fails only when the
  // process loses its connection to the render service, which leaves no
  // vsyncs to pace with.
  if (OH_NativeVSync_RequestFrame(side_->vsync, on_frame, side_.get()) != 0) {
    const auto lock = std::scoped_lock{side_->mutex};
    side_->request_hold.reset();
  }
}

}  // namespace

auto create_frame_pacer(
  const mln_frame_pacer_options* options, mln_frame_pacer* out_pacer
) -> mln_status {
  auto minimum_interval = std::chrono::nanoseconds{};
  const auto options_status =
    core::frame_pacer_minimum_interval(options, minimum_interval);
  if (options_status != MLN_STATUS_OK) return options_status;
  const auto output_status = core::validate_frame_pacer_output(out_pacer);
  if (output_status != MLN_STATUS_OK) return output_status;
  constexpr auto name = std::string_view{"maplibre-native-ffi"};
  auto* const vsync = OH_NativeVSync_Create(name.data(), name.size());
  if (vsync == nullptr) {
    core::set_thread_error(
      "the frame pacer could not create an OH_NativeVSync"
    );
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto side = std::shared_ptr<VSyncSide>{};
  try {
    side = std::make_shared<VSyncSide>(vsync);
  } catch (...) {
    OH_NativeVSync_Destroy(vsync);
    throw;
  }
  auto pacer = std::make_shared<NativeVSyncPacer>(minimum_interval, side);
  side->pacer = pacer;
  return core::frame_pacer_insert(std::move(pacer), out_pacer);
}

}  // namespace mln::platform::ohos
