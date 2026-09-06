#include <string>

#include <mln/platform/thread.hpp>
#include <mln/util/platform.hpp>

namespace mln {
namespace platform {

// No-ops in browser builds: Emscripten exposes no scheduling priority APIs and
// no pthread_setname_np. Only diagnostics are affected.

std::string getCurrentThreadName() { return "emscripten"; }

void setCurrentThreadName(const std::string&) {}

void makeThreadLowPriority() {}

void setCurrentThreadPriority(double) {}

void attachThread() {}

void detachThread() {}

}  // namespace platform
}  // namespace mln
