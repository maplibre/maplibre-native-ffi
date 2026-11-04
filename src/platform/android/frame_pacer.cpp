// A frame pacer on the AChoreographer of the thread that creates it.
//
// AChoreographer works only on its looper's thread, so a request from another
// thread signals an eventfd that the looper watches, and the watch's callback
// posts the frame callback. A posted frame callback can't be cancelled, and the
// looper keeps calling the watch's callback until it ends the watch, so each of
// them holds the looper side of the pacer until it runs for the last time.

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

#include <android/choreographer.h>
#include <android/looper.h>
#include <dlfcn.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#include "platform/android/frame_pacer.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/frame_pacer.hpp"

namespace mln::platform::android {

namespace {

class ChoreographerPacer;

// What the looper thread's callbacks share with the pacer. The watch of the
// eventfd and the posted frame callback each hold it until they end, so it
// can outlive the pacer.
struct LooperSide final : std::enable_shared_from_this<LooperSide> {
  LooperSide(
    ALooper* looper_, AChoreographer* choreographer_, int event_fd_,
    pid_t thread_
  ) noexcept
      : looper{looper_},
        choreographer{choreographer_},
        event_fd{event_fd_},
        thread{thread_} {}
  LooperSide(const LooperSide&) = delete;
  LooperSide(LooperSide&&) = delete;
  auto operator=(const LooperSide&) -> LooperSide& = delete;
  auto operator=(LooperSide&&) -> LooperSide& = delete;
  ~LooperSide() { close(event_fd); }

  ALooper* const looper;
  AChoreographer* const choreographer;
  const int event_fd;
  const pid_t thread;
  // Set before the watch starts.
  std::weak_ptr<ChoreographerPacer> pacer;
  std::atomic_bool stopping = false;
  // Used only on the looper's thread.
  std::shared_ptr<LooperSide> watch_hold;
  std::shared_ptr<LooperSide> frame_hold;
};

using PostFrameCallback64 =
  void (*)(AChoreographer*, AChoreographer_frameCallback64, void*);

// API level 29 added AChoreographer_postFrameCallback64(). The minimum API
// level's AChoreographer_postFrameCallback() passes the frame time as a long,
// which is 32 bits in a 32-bit process.
auto post_frame_callback64() noexcept -> PostFrameCallback64 {
  static const auto function = reinterpret_cast<PostFrameCallback64>(
    dlsym(RTLD_DEFAULT, "AChoreographer_postFrameCallback64")
  );
  return function;
}

class ChoreographerPacer final : public core::FramePacer {
 public:
  ChoreographerPacer(
    std::chrono::nanoseconds minimum_interval,
    std::shared_ptr<LooperSide> looper
  ) noexcept
      : FramePacer{minimum_interval}, looper_{std::move(looper)} {}
  ChoreographerPacer(const ChoreographerPacer&) = delete;
  ChoreographerPacer(ChoreographerPacer&&) = delete;
  auto operator=(const ChoreographerPacer&) -> ChoreographerPacer& = delete;
  auto operator=(ChoreographerPacer&&) -> ChoreographerPacer& = delete;
  // Ends the watch, which lets go of the looper side once it sees the signal.
  ~ChoreographerPacer() override {
    looper_->stopping.store(true);
    signal();
  }

  // Runs on the looper's thread at a vsync.
  auto vsync(std::chrono::nanoseconds time) noexcept -> void { on_vsync(time); }

 protected:
  auto schedule_vsync() noexcept -> void override;

 private:
  auto signal() const noexcept -> void {
    const auto one = std::uint64_t{1};
    // A write that fails because the counter is full still leaves the looper
    // a signal to read.
    while (write(looper_->event_fd, &one, sizeof(one)) < 0 && errno == EINTR) {
    }
  }

  std::shared_ptr<LooperSide> looper_;
};

auto run_frame(LooperSide& looper, std::chrono::nanoseconds time) noexcept
  -> void {
  // Declared first, so it lets go of the looper side last.
  const auto hold = std::move(looper.frame_hold);
  if (const auto pacer = looper.pacer.lock()) pacer->vsync(time);
}

void on_frame64(int64_t frame_time_ns, void* data) {
  run_frame(
    *static_cast<LooperSide*>(data), std::chrono::nanoseconds{frame_time_ns}
  );
}

// A 32-bit long carries only the low bits of the frame time, so the upper bits
// come from CLOCK_MONOTONIC, the frame time's clock, as the NDK recommends.
void on_frame(long frame_time_ns, void* data) {
  auto time = static_cast<int64_t>(frame_time_ns);
  if constexpr (sizeof(long) < sizeof(int64_t)) {
    auto now = timespec{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    const auto now_ns =
      (static_cast<int64_t>(now.tv_sec) * 1'000'000'000) + now.tv_nsec;
    constexpr auto low_bits = int64_t{0xffffffff};
    time =
      (now_ns & ~low_bits) | (static_cast<int64_t>(frame_time_ns) & low_bits);
    if (time > now_ns) time -= low_bits + 1;
  }
  run_frame(*static_cast<LooperSide*>(data), std::chrono::nanoseconds{time});
}

// Runs on the looper's thread.
auto post_frame(LooperSide& looper) noexcept -> void {
  if (looper.frame_hold != nullptr) return;
  looper.frame_hold = looper.shared_from_this();
  if (const auto post64 = post_frame_callback64(); post64 != nullptr) {
    post64(looper.choreographer, on_frame64, &looper);
  } else {
    AChoreographer_postFrameCallback(looper.choreographer, on_frame, &looper);
  }
}

auto ChoreographerPacer::schedule_vsync() noexcept -> void {
  if (gettid() == looper_->thread) {
    post_frame(*looper_);
  } else {
    signal();
  }
}

int on_signal(int fd, int /*events*/, void* data) {
  auto& looper = *static_cast<LooperSide*>(data);
  auto count = std::uint64_t{};
  static_cast<void>(read(fd, &count, sizeof(count)));
  if (!looper.stopping.load()) {
    post_frame(looper);
    return 1;
  }
  // Ends the watch before the looper side can close the eventfd, which the
  // looper would otherwise still watch when this returns.
  ALooper_removeFd(looper.looper, fd);
  const auto hold = std::move(looper.watch_hold);
  return 0;
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
  auto* const looper = ALooper_forThread();
  if (looper == nullptr) {
    core::set_thread_error("the calling thread has no ALooper");
    return MLN_STATUS_INVALID_STATE;
  }
  auto* const choreographer = AChoreographer_getInstance();
  if (choreographer == nullptr) {
    core::set_thread_error("the calling thread has no AChoreographer");
    return MLN_STATUS_NATIVE_ERROR;
  }
  const auto event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (event_fd < 0) {
    core::set_thread_error("the frame pacer could not create an eventfd");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto side = std::shared_ptr<LooperSide>{};
  try {
    side =
      std::make_shared<LooperSide>(looper, choreographer, event_fd, gettid());
  } catch (...) {
    close(event_fd);
    throw;
  }
  auto pacer = std::make_shared<ChoreographerPacer>(minimum_interval, side);
  side->pacer = pacer;
  side->watch_hold = side;
  if (
    ALooper_addFd(
      looper, event_fd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, on_signal,
      side.get()
    ) != 1
  ) {
    side->watch_hold.reset();
    core::set_thread_error("the frame pacer could not watch its eventfd");
    return MLN_STATUS_NATIVE_ERROR;
  }
  return core::frame_pacer_insert(std::move(pacer), out_pacer);
}

}  // namespace mln::platform::android
