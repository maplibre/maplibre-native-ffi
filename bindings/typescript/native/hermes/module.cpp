#include "module.h"

#include "shim.h"

extern "C" napi_value napi_register_module_v1(napi_env, napi_value);

namespace mln::hermes {

Module::Module(facebook::jsi::Runtime& runtime, mln::hermes::Schedule schedule)
    : env_(std::make_unique<Environment>(runtime, std::move(schedule))) {
  try {
    runtime.global().setProperty(
      runtime, "__mlnNativeFfi", env_->install(napi_register_module_v1)
    );
  } catch (...) {
    env_->cleanup();
    throw;
  }
}

Module::~Module() { cleanup(); }

void Module::cleanup() { env_->cleanup(); }

}  // namespace mln::hermes
