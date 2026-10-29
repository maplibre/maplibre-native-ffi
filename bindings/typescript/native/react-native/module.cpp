#include "module.h"

extern "C" napi_value napi_register_module_v1(napi_env, napi_value);

namespace mln::react_native {

Module::Module(
  facebook::jsi::Runtime& runtime,
  std::shared_ptr<facebook::react::CallInvoker> invoker
)
    : env_(runtime, std::move(invoker)) {
  try {
    runtime.global().setProperty(
      runtime, "__mlnNativeFfi", env_.install(napi_register_module_v1)
    );
  } catch (...) {
    env_.cleanup();
    throw;
  }
}

void Module::cleanup() { env_.cleanup(); }

}  // namespace mln::react_native
