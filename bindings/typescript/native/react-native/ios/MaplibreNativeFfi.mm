#import <MaplibreNativeFfiSpec/MaplibreNativeFfiSpec.h>
#import <React/RCTBridge+Private.h>
#import <React/RCTInvalidating.h>
#import <ReactCommon/RCTTurboModuleWithJSIBindings.h>

#include "module.h"

@interface MaplibreNativeFfi : NSObject <
                                 NativeMaplibreNativeFfiSpec,
                                 RCTTurboModuleWithJSIBindings,
                                 RCTInvalidating>
@end

@implementation MaplibreNativeFfi {
  std::unique_ptr<mln::react_native::Module> _module;
}

RCT_EXPORT_MODULE(MaplibreNativeFfi)

+ (BOOL)requiresMainQueueSetup {
  return NO;
}
- (dispatch_queue_t)methodQueue {
  return RCTJSThread;
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
  (const facebook::react::ObjCTurboModule::InitParams&)params {
  return std::make_shared<facebook::react::NativeMaplibreNativeFfiSpecJSI>(
    params
  );
}

- (void)
  installJSIBindingsWithRuntime:(facebook::jsi::Runtime&)runtime
                    callInvoker:
                      (
                        const std::shared_ptr<facebook::react::CallInvoker>&
                      )invoker {
  _module = std::make_unique<mln::react_native::Module>(runtime, invoker);
}

- (void)invalidate {
  if (_module) _module->cleanup();
  _module.reset();
}

@end
