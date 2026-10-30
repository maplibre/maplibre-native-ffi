#pragma once

#include <memory>

#include <jsi/jsi.h>
#include <node_api.h>

#include "module.h"
#include "symbols.h"

namespace mln::hermes {

class Environment {
 public:
  Environment(facebook::jsi::Runtime& runtime, mln::hermes::Schedule schedule);
  ~Environment();
  napi_env get() const;
  // The platform calls this on the installation thread, before runtime
  // destruction, even for release-mode reloads. No JSI root survives it.
  void cleanup();
  facebook::jsi::Object install(napi_value (*module)(napi_env, napi_value));

 private:
  std::shared_ptr<napi_env__> env_;
};

}  // namespace mln::hermes
