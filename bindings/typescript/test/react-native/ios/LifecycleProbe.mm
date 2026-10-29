#import <LifecycleProbeSpec/LifecycleProbeSpec.h>
#import <React/RCTBridge+Private.h>
#import <React/RCTInvalidating.h>
#import <React/RCTReloadCommand.h>
#import <ReactCommon/RCTTurboModuleWithJSIBindings.h>

#include "Probe.h"

@interface LifecycleProbe : NSObject <
                              NativeLifecycleProbeSpec,
                              RCTTurboModuleWithJSIBindings,
                              RCTInvalidating>
@end

@implementation LifecycleProbe {
  std::unique_ptr<mln::test::Probe> _probe;
}

RCT_EXPORT_MODULE(LifecycleProbe)

+ (BOOL)requiresMainQueueSetup {
  return NO;
}
- (dispatch_queue_t)methodQueue {
  return RCTJSThread;
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
  (const facebook::react::ObjCTurboModule::InitParams&)params {
  return std::make_shared<facebook::react::NativeLifecycleProbeSpecJSI>(params);
}

- (void)
  installJSIBindingsWithRuntime:(facebook::jsi::Runtime&)runtime
                    callInvoker:
                      (
                        const std::shared_ptr<facebook::react::CallInvoker>&
                      )invoker {
  _probe = std::make_unique<mln::test::Probe>(runtime, invoker);
}

- (void)reload {
  dispatch_async(dispatch_get_main_queue(), ^{
    RCTTriggerReloadCommandListeners(@"MapLibre lifecycle probe");
  });
}

- (void)invalidate {
  _probe->cleanup();
  _probe.reset();
}

@end
