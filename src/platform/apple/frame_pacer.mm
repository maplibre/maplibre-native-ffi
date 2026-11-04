// A frame pacer on a CADisplayLink of its own.
//
// Every pacer's display link runs on one thread that the library starts for
// them, so a pacer can be created and asked for vsyncs on any thread, and
// needs no run loop of the host's. A display link and its run loop aren't
// thread-safe, so the pacer touches its link only on that thread: a request
// from another thread performs a block there. A display link retains its
// target until the link is invalidated, so the target holds the pacer weakly,
// and a pacer that goes away invalidates its link on that thread.

#include <chrono>
#include <memory>
#include <utility>

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#include <TargetConditionals.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <pthread/qos.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#endif

#include "diagnostics/diagnostics.hpp"
#include "execution/worker_thread.hpp"
#include "maplibre_native_c.h"
#include "platform/apple/frame_pacer.hpp"
#include "render/frame_pacer.hpp"

namespace mln::platform::apple {

// The thread that runs every pacer's display link, on a run loop that runs
// for the rest of the process.
class LinkThread final {
 public:
  LinkThread(const LinkThread&) = delete;
  LinkThread(LinkThread&&) = delete;
  auto operator=(const LinkThread&) -> LinkThread& = delete;
  auto operator=(LinkThread&&) -> LinkThread& = delete;
  ~LinkThread() = default;

  // Starts the thread on first use. Throws std::system_error when it can't.
  static auto instance() -> LinkThread& {
    static auto thread = LinkThread{};
    return thread;
  }

  [[nodiscard]] auto is_current() const noexcept -> bool {
    return pthread_equal(pthread_self(), thread_) != 0;
  }

  // Runs `block` on the thread, after the blocks performed before it.
  auto perform(dispatch_block_t block) const noexcept -> void {
    CFRunLoopPerformBlock(run_loop_, kCFRunLoopDefaultMode, block);
    CFRunLoopWakeUp(run_loop_);
  }

  // Runs `block` on the thread and waits for it to return.
  auto perform_and_wait(dispatch_block_t block) const noexcept -> void {
    if (is_current()) {
      block();
      return;
    }
    dispatch_semaphore_t const done = dispatch_semaphore_create(0);
    perform(^{
      block();
      dispatch_semaphore_signal(done);
    });
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
  }

 private:
  LinkThread() {
    dispatch_semaphore_t const ready = dispatch_semaphore_create(0);
    auto worker = core::WorkerThread{[this, ready] { run(ready); }};
    worker.detach();
    dispatch_semaphore_wait(ready, DISPATCH_TIME_FOREVER);
  }

  auto run(dispatch_semaphore_t ready) noexcept -> void {
    static_cast<void>(pthread_setname_np("MLN frame pacer"));
    // A vsync that waits behind other work delays the frames it demands.
    static_cast<void>(
      pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)
    );
    thread_ = pthread_self();
    run_loop_ = CFRunLoopGetCurrent();
    // A run loop returns at once when its mode has no source, as it has
    // while no pacer has a display link, so a source that never fires keeps
    // it running.
    auto context = CFRunLoopSourceContext{};
    CFRunLoopSourceRef const idle =
      CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    CFRunLoopAddSource(run_loop_, idle, kCFRunLoopDefaultMode);
    CFRelease(idle);
    dispatch_semaphore_signal(ready);
    while (true) CFRunLoopRun();
  }

  pthread_t thread_{};
  CFRunLoopRef run_loop_ = nullptr;
};

class DisplayLinkPacer final : public core::FramePacer {
 public:
  DisplayLinkPacer(
    std::chrono::nanoseconds minimum_interval, const LinkThread& thread
  ) noexcept
      : FramePacer{minimum_interval}, thread_{&thread} {}
  DisplayLinkPacer(const DisplayLinkPacer&) = delete;
  DisplayLinkPacer(DisplayLinkPacer&&) = delete;
  auto operator=(const DisplayLinkPacer&) -> DisplayLinkPacer& = delete;
  auto operator=(DisplayLinkPacer&&) -> DisplayLinkPacer& = delete;
  // Invalidates the link, which lets go of its target.
  ~DisplayLinkPacer() override {
    if (link_ == nil) return;
    CADisplayLink* const link = link_;
    thread_->perform(^{
      [link invalidate];
    });
  }

  // Called once, before the pacer is published.
  auto set_link(CADisplayLink* link) noexcept -> void { link_ = link; }

  // Runs on the link's thread at a vsync.
  auto vsync(std::chrono::nanoseconds time) noexcept -> void { on_vsync(time); }

 protected:
  auto schedule_vsync() noexcept -> void override {
    if (thread_->is_current()) {
      link_.paused = NO;
      return;
    }
    CADisplayLink* const link = link_;
    thread_->perform(^{
      link.paused = NO;
    });
  }

 private:
  const LinkThread* thread_;
  CADisplayLink* link_ = nil;
};

}  // namespace mln::platform::apple

// The display link's target. The link retains it until the link is
// invalidated, which can be after the pacer is gone.
@interface MLNFFIFramePacerTarget : NSObject
- (instancetype)initWithPacer:
  (const std::weak_ptr<mln::platform::apple::DisplayLinkPacer>&)pacer;
- (void)vsync:(CADisplayLink*)link;
@end

@implementation MLNFFIFramePacerTarget {
  std::weak_ptr<mln::platform::apple::DisplayLinkPacer> _pacer;
}

- (instancetype)initWithPacer:
  (const std::weak_ptr<mln::platform::apple::DisplayLinkPacer>&)pacer {
  self = [super init];
  if (self) _pacer = pacer;
  return self;
}

- (void)vsync:(CADisplayLink*)link {
  @autoreleasepool {
    // A paused link calls back no more until a request resumes it.
    link.paused = YES;
    if (const auto pacer = _pacer.lock()) {
      // The timestamp is in seconds on the host's monotonic clock.
      pacer->vsync(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::duration<double>{link.timestamp}
        )
      );
    }
  }
}

@end

namespace mln::platform::apple {

namespace {

// Runs on the link thread.
auto make_display_link(MLNFFIFramePacerTarget* target) -> CADisplayLink* {
#if TARGET_OS_OSX
  // The first screen is the main display, the one with the menu bar.
  NSScreen* const screen = NSScreen.screens.firstObject;
  if (screen == nil) return nil;
  return [screen displayLinkWithTarget:target selector:@selector(vsync:)];
#else
  return [CADisplayLink displayLinkWithTarget:target
                                     selector:@selector(vsync:)];
#endif
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
  auto rate = 0.0F;
  if (
    options != nullptr &&
    (options->fields & MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND) != 0U
  )
    rate = static_cast<float>(options->maximum_frames_per_second);

  const auto& thread = LinkThread::instance();
  auto pacer = std::make_shared<DisplayLinkPacer>(minimum_interval, thread);
  MLNFFIFramePacerTarget* const target =
    [[MLNFFIFramePacerTarget alloc] initWithPacer:pacer];
  __block CADisplayLink* link = nil;
  thread.perform_and_wait(^{
    @autoreleasepool {
      link = make_display_link(target);
      if (link == nil) return;
      link.paused = YES;
      // Lets a display with a variable refresh rate slow down to the limit.
      if (rate > 0.0F)
        link.preferredFrameRateRange = CAFrameRateRangeMake(rate, rate, rate);
      [link addToRunLoop:NSRunLoop.currentRunLoop forMode:NSDefaultRunLoopMode];
    }
  });
  if (link == nil) {
    core::set_thread_error("the system has no display for the pacer to follow");
    return MLN_STATUS_NATIVE_ERROR;
  }
  pacer->set_link(link);
  return core::frame_pacer_insert(std::move(pacer), out_pacer);
}

}  // namespace mln::platform::apple
