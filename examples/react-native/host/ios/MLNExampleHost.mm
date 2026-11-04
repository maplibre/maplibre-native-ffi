#import <MaplibreExampleHostSpec/MaplibreExampleHostSpec.h>
#import <React/RCTBridgeModule.h>
#import <React/RCTInvalidating.h>
#import <ReactCommon/RCTTurboModuleWithJSIBindings.h>

#import "MLNExampleHost.h"

#include "Host.h"

using namespace facebook;

@interface MLNExampleHost () <
  NativeMaplibreHostSpec,
  RCTTurboModuleWithJSIBindings,
  RCTInvalidating>
@end

@implementation MLNExampleHost

RCT_EXPORT_MODULE(MaplibreHost)

// React Native invalidates a module on its method queue, and runs this queue's
// invalidation on the JavaScript thread before it destroys the runtime.
- (dispatch_queue_t)methodQueue {
  return RCTJSThread;
}

- (std::shared_ptr<react::TurboModule>)getTurboModule:
  (const react::ObjCTurboModule::InitParams&)params {
  return std::make_shared<react::NativeMaplibreHostSpecJSI>(params);
}

- (void)
  installJSIBindingsWithRuntime:(jsi::Runtime&)runtime
                    callInvoker:
                      (const std::shared_ptr<react::CallInvoker>&)callInvoker {
  mln::example::install(runtime, callInvoker);
}

- (void)invalidate {
  mln::example::cleanup();
}

@end
