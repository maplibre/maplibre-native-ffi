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

auto finish(mln_test_completion& completion) -> bool {
  const auto status = mln_test_completion_finish(&completion);
  mln_test_completion_destroy(&completion);
  return status == MLN_STATUS_OK;
}

}  // namespace

extern "C" bool mln_test_metal_surface_retarget_retains_submission(
  mln_map map
) {
  @autoreleasepool {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    CAMetalLayer* initial_layer = [[CAMetalLayer alloc] init];
    if (device == nil || initial_layer == nil) return false;

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
      ) != MLN_STATUS_OK ||
      !finish(attach)
    ) {
      return false;
    }
    initial_layer = nil;

    atomic_bool entered = false;
    atomic_bool release = false;
    auto blocker = mln_test_completion_default(0);
    if (
      mln_test_render_session_blocking_operation_create(
        session, &entered, &release, &blocker.descriptor
      ) != MLN_STATUS_OK
    ) {
      return false;
    }
    for (unsigned int attempt = 0; attempt < 10000 && !atomic_load(&entered);
         ++attempt) {
      mln_test_sleep_millisecond();
    }
    if (!atomic_load(&entered)) return false;

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
    const auto retained_before_driver_execution =
      !replacement_deallocated.load();

    atomic_store(&release, true);
    const auto blocker_finished = finish(blocker);
    const auto replacement_finished =
      replacement_status == MLN_STATUS_OK && finish(replacement);
    if (replacement_status != MLN_STATUS_OK) {
      mln_test_completion_reject(&replacement);
      mln_test_completion_destroy(&replacement);
    }

    auto detach = mln_test_completion_default(0);
    const auto detach_status =
      mln_render_session_detach(session, &detach.descriptor);
    const auto detached = detach_status == MLN_STATUS_OK && finish(detach);
    if (detach_status != MLN_STATUS_OK) {
      mln_test_completion_reject(&detach);
      mln_test_completion_destroy(&detach);
    }
    const auto destroyed = mln_render_session_destroy(session) == MLN_STATUS_OK;
    if (weak_replacement_layer != nil) {
      objc_setAssociatedObject(
        weak_replacement_layer, &deallocation_probe_key, nil,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
      );
    }
    return retained_before_driver_execution && blocker_finished &&
           replacement_finished && detached && destroyed;
  }
}
