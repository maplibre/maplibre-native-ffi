#include <atomic>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <objc/runtime.h>

#include "test_support.h"

@interface MLNTestDeallocationProbe : NSObject

@property(nonatomic, assign) std::atomic_bool* deallocated;

@end

@implementation MLNTestDeallocationProbe

- (void)dealloc {
  self.deallocated->store(true);
}

@end

namespace {

char deallocation_probe_key;

// Settles a completion the caller submitted successfully.
auto finish(mln_test_completion& completion) -> bool {
  const auto status = mln_test_completion_finish(&completion);
  mln_test_completion_destroy(&completion);
  return status == MLN_STATUS_OK;
}

// Settles a completion a rejected submission left with the caller.
void discard(mln_test_completion& completion) {
  mln_test_completion_reject(&completion);
  mln_test_completion_destroy(&completion);
}

// Detaches and destroys the session on every exit path, and reports the first
// failure the caller saw, or its own.
auto teardown(mln_render_session session, const char* failure) -> const char* {
  auto detach = mln_test_completion_default(0);
  if (mln_render_session_detach(session, &detach.descriptor) != MLN_STATUS_OK) {
    discard(detach);
    if (failure == nullptr) failure = "the session detach was rejected";
  } else if (!finish(detach) && failure == nullptr) {
    failure = "the session detach failed";
  }
  if (
    mln_render_session_destroy(session) != MLN_STATUS_OK && failure == nullptr
  ) {
    failure = "the session destroy was rejected";
  }
  return failure;
}

}  // namespace

extern "C" auto mln_test_metal_surface_retarget_retains_submission(mln_map map)
  -> const char* {
  @autoreleasepool {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    CAMetalLayer* initial_layer = [[CAMetalLayer alloc] init];
    if (device == nil || initial_layer == nil) {
      return "this host has no Metal device or layer";
    }

    auto descriptor = mln_metal_surface_descriptor_default();
    descriptor.context.device = (__bridge void*)device;
    descriptor.layer = (__bridge void*)initial_layer;
    auto options = mln_render_session_attach_options_default();
    options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
    mln_render_session session = MLN_HANDLE_NULL;
    auto attach = mln_test_completion_default(0);
    if (
      mln_metal_surface_attach(
        map, &descriptor, &options, &session, &attach.descriptor
      ) != MLN_STATUS_OK
    ) {
      discard(attach);
      return "the Metal surface attach was rejected";
    }
    const char* failure =
      finish(attach) ? nullptr : "the attach completion failed";
    initial_layer = nil;

    // Every later step runs behind one blocking driver operation, so the tail
    // below has to release it and settle its completion on every path.
    atomic_bool entered = false;
    atomic_bool release = false;
    auto blocker = mln_test_completion_default(0);
    if (
      failure != nullptr || mln_test_render_session_blocking_operation_create(
                              session, &entered, &release, &blocker.descriptor
                            ) != MLN_STATUS_OK
    ) {
      discard(blocker);
      if (failure == nullptr) {
        failure = "the blocking driver operation was rejected";
      }
      return teardown(session, failure);
    }
    if (!mln_test_wait_for_flag(&entered)) {
      atomic_store(&release, true);
      static_cast<void>(finish(blocker));
      return teardown(session, "the blocking driver operation never ran");
    }

    std::atomic_bool replacement_deallocated = false;
    CAMetalLayer* replacement_layer = [[CAMetalLayer alloc] init];
    __weak CAMetalLayer* weak_replacement_layer = replacement_layer;
    auto* probe = [[MLNTestDeallocationProbe alloc] init];
    probe.deallocated = &replacement_deallocated;
    objc_setAssociatedObject(
      replacement_layer, &deallocation_probe_key, probe,
      OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
    probe = nil;
    descriptor.layer = (__bridge void*)replacement_layer;
    auto replacement = mln_test_completion_default(0);
    const auto replacement_status = mln_metal_surface_set_target(
      session, &descriptor, &replacement.descriptor
    );
    replacement_layer = nil;
    if (replacement_deallocated.load()) {
      failure =
        "the retarget released the replacement layer before the driver "
        "ran it";
    }

    atomic_store(&release, true);
    if (!finish(blocker) && failure == nullptr) {
      failure = "the blocking driver operation did not complete";
    }
    if (replacement_status != MLN_STATUS_OK) {
      discard(replacement);
      if (failure == nullptr) failure = "the Metal retarget was rejected";
    } else if (!finish(replacement) && failure == nullptr) {
      failure = "the retarget completion failed";
    }

    if (weak_replacement_layer != nil) {
      objc_setAssociatedObject(
        weak_replacement_layer, &deallocation_probe_key, nil,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
      );
    }
    return teardown(session, failure);
  }
}
