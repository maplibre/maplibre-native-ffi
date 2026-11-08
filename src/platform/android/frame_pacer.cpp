// A frame pacer on the AChoreographer of the thread that creates it.
//
// AChoreographer works only on its looper's thread, so a request from another
// thread signals an eventfd that the looper watches, and the watch's callback
// posts the frame callback.
//
// Choreographer can stop while the phone's display is off, even while a
// Surface on another display, such as a car's, is still on screen. So each
// request also arms a timerfd that the looper watches, and the timer demands
// the frame when the vsync is late. After the timer demands a frame, it runs
// every timer_frame_interval until a vsync arrives first again.
//
// A posted frame callback can't be cancelled, and the looper keeps calling a
// watch's callback until it ends the watch, so each of them holds the looper
// side of the pacer until it runs for the last time.

#include <algorithm>
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
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "platform/android/frame_pacer.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/frame_pacer.hpp"

namespace mln::platform::android {

namespace {

// Longer than a vsync period at the lowest refresh rate that a phone display
// runs while it is on.
constexpr auto vsync_timeout = std::chrono::milliseconds{50};
constexpr auto timer_frame_interval = std::chrono::milliseconds{16};

class ChoreographerPacer;

// What the looper thread's callbacks share with the pacer. The watches of the
// eventfd and the timerfd and the posted frame callback each hold it until
// they end, so it can outlive the pacer.
struct LooperSide final : std::enable_shared_from_this<LooperSide> {
  LooperSide(
    ALooper* looper_, AChoreographer* choreographer_, int event_fd_,
    int timer_fd_, pid_t thread_
  ) noexcept
      : looper{looper_},
        choreographer{choreographer_},
        event_fd{event_fd_},
        timer_fd{timer_fd_},
        thread{thread_} {}
  LooperSide(const LooperSide&) = delete;
  LooperSide(LooperSide&&) = delete;
  auto operator=(const LooperSide&) -> LooperSide& = delete;
  auto operator=(LooperSide&&) -> LooperSide& = delete;
  ~LooperSide() {
    close(event_fd);
    close(timer_fd);
  }

  ALooper* const looper;
  AChoreographer* const choreographer;
  const int event_fd;
  const int timer_fd;
  const pid_t thread;
  // Set before the watches start.
  std::weak_ptr<ChoreographerPacer> pacer;
  std::atomic_bool stopping = false;
  // Used only on the looper's thread.
  std::shared_ptr<LooperSide> watch_hold;
  std::shared_ptr<LooperSide> timer_hold;
  std::shared_ptr<LooperSide> frame_hold;
  // Whether the pacer waits for a frame that neither the vsync nor the timer
  // has sent.
  bool requested = false;
  // Whether the timer sent the last frame.
  bool vsync_stopped = false;
  std::chrono::nanoseconds last_frame_time = std::chrono::nanoseconds::min();
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

// The clock of a frame callback's frame time.
auto monotonic_now() noexcept -> std::chrono::nanoseconds {
  auto now = timespec{};
  clock_gettime(CLOCK_MONOTONIC, &now);
  return std::chrono::seconds{now.tv_sec} +
         std::chrono::nanoseconds{now.tv_nsec};
}

// A zero delay disarms the timer.
auto arm_timer(int timer_fd, std::chrono::nanoseconds delay) noexcept -> void {
  auto spec = itimerspec{};
  const auto seconds = std::chrono::floor<std::chrono::seconds>(delay);
  spec.it_value.tv_sec = static_cast<time_t>(seconds.count());
  spec.it_value.tv_nsec = static_cast<long>((delay - seconds).count());
  static_cast<void>(timerfd_settime(timer_fd, 0, &spec, nullptr));
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
  // Ends the watches, which let go of the looper side once they see the
  // signal.
  ~ChoreographerPacer() override {
    looper_->stopping.store(true);
    signal();
  }

  // Runs on the looper's thread at a vsync or a timer frame.
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

// Runs on the looper's thread.
auto send_frame(
  LooperSide& looper, std::chrono::nanoseconds time, bool from_vsync
) noexcept -> void {
  looper.requested = false;
  looper.vsync_stopped = !from_vsync;
  arm_timer(looper.timer_fd, std::chrono::nanoseconds::zero());
  // A vsync's time can precede the time of a timer frame sent just before it.
  looper.last_frame_time =
    std::max(time, looper.last_frame_time + std::chrono::nanoseconds{1});
  if (const auto pacer = looper.pacer.lock()) {
    pacer->vsync(looper.last_frame_time);
  }
}

auto run_frame(LooperSide& looper, std::chrono::nanoseconds time) noexcept
  -> void {
  // Declared first, so it lets go of the looper side last.
  const auto hold = std::move(looper.frame_hold);
  // Leaves armed the timer that ends the timer's watch.
  if (looper.stopping.load()) return;
  if (looper.requested) {
    send_frame(looper, time, true);
  } else {
    // A frame callback posted before the timer took over shows that vsyncs
    // arrive again.
    looper.vsync_stopped = false;
  }
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
    const auto now_ns = monotonic_now().count();
    constexpr auto low_bits = int64_t{0xffffffff};
    time =
      (now_ns & ~low_bits) | (static_cast<int64_t>(frame_time_ns) & low_bits);
    if (time > now_ns) time -= low_bits + 1;
  }
  run_frame(*static_cast<LooperSide*>(data), std::chrono::nanoseconds{time});
}

// Runs on the looper's thread.
auto request_looper_frame(LooperSide& looper) noexcept -> void {
  if (looper.requested) return;
  looper.requested = true;
  // A frame callback that is still posted serves this request.
  if (looper.frame_hold == nullptr) {
    looper.frame_hold = looper.shared_from_this();
    if (const auto post64 = post_frame_callback64(); post64 != nullptr) {
      post64(looper.choreographer, on_frame64, &looper);
    } else {
      AChoreographer_postFrameCallback(looper.choreographer, on_frame, &looper);
    }
  }
  arm_timer(
    looper.timer_fd, looper.vsync_stopped ? timer_frame_interval : vsync_timeout
  );
}

auto ChoreographerPacer::schedule_vsync() noexcept -> void {
  if (gettid() == looper_->thread) {
    request_looper_frame(*looper_);
  } else {
    signal();
  }
}

int on_signal(int fd, int /*events*/, void* data) {
  auto& looper = *static_cast<LooperSide*>(data);
  auto count = std::uint64_t{};
  static_cast<void>(read(fd, &count, sizeof(count)));
  if (!looper.stopping.load()) {
    request_looper_frame(looper);
    return 1;
  }
  // Wakes the timer's watch, so that it ends too.
  arm_timer(looper.timer_fd, std::chrono::nanoseconds{1});
  // Ends the watch before the looper side can close the eventfd, which the
  // looper would otherwise still watch when this returns.
  ALooper_removeFd(looper.looper, fd);
  const auto hold = std::move(looper.watch_hold);
  return 0;
}

int on_timer(int fd, int /*events*/, void* data) {
  auto& looper = *static_cast<LooperSide*>(data);
  auto count = std::uint64_t{};
  // A timer that a frame disarmed after it expired has nothing to read.
  const auto expired = read(fd, &count, sizeof(count)) == sizeof(count);
  if (!looper.stopping.load()) {
    if (expired && looper.requested) send_frame(looper, monotonic_now(), false);
    return 1;
  }
  ALooper_removeFd(looper.looper, fd);
  const auto hold = std::move(looper.timer_hold);
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
  const auto timer_fd =
    timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
  if (timer_fd < 0) {
    close(event_fd);
    core::set_thread_error("the frame pacer could not create a timerfd");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto side = std::shared_ptr<LooperSide>{};
  try {
    side = std::make_shared<LooperSide>(
      looper, choreographer, event_fd, timer_fd, gettid()
    );
  } catch (...) {
    close(event_fd);
    close(timer_fd);
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
  side->timer_hold = side;
  if (
    ALooper_addFd(
      looper, timer_fd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, on_timer,
      side.get()
    ) != 1
  ) {
    side->timer_hold.reset();
    core::set_thread_error("the frame pacer could not watch its timerfd");
    // Destroying the pacer ends the eventfd's watch.
    return MLN_STATUS_NATIVE_ERROR;
  }
  return core::frame_pacer_insert(std::move(pacer), out_pacer);
}

}  // namespace mln::platform::android
