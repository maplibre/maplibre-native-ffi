#import <MaplibreNativeFfiReactNativeSpec/MaplibreNativeFfiReactNativeSpec.h>
#import <React/RCTBridgeModule.h>
#import <React/RCTInvalidating.h>
#import <ReactCommon/RCTTurboModuleWithJSIBindings.h>

#import "MLNReactNativeModule.h"

#include "Host.h"

using namespace facebook;

@interface MLNReactNativeModule () <
  NativeMaplibreNativeFfiSpec,
  RCTTurboModuleWithJSIBindings,
  RCTInvalidating>
@end

@implementation MLNReactNativeModule

RCT_EXPORT_MODULE(MaplibreNativeFfi)

// React Native invalidates a module on its method queue, and runs this queue's
// invalidation on the JavaScript thread before it destroys the runtime.
- (dispatch_queue_t)methodQueue {
  return RCTJSThread;
}

- (std::shared_ptr<react::TurboModule>)getTurboModule:
  (const react::ObjCTurboModule::InitParams&)params {
  return std::make_shared<react::NativeMaplibreNativeFfiSpecJSI>(params);
}

- (void)
  installJSIBindingsWithRuntime:(jsi::Runtime&)runtime
                    callInvoker:
                      (const std::shared_ptr<react::CallInvoker>&)callInvoker {
  mln::react_native::install(runtime, callInvoker);
}

- (void)invalidate {
  mln::react_native::cleanup();
}

@end
