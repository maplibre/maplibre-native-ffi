#include <string>

#include <mbgl/platform/thread.hpp>
#include <mbgl/util/platform.hpp>

namespace mbgl {
namespace platform {

// MapLibre calls these for worker-thread lifecycle and priority tuning. On
// Emscripten they are deliberate no-ops: the host does not expose Linux-style
// sched_setscheduler/setpriority APIs, and pthread_setname_np is unavailable
// in browser builds. ThreadPool/util::Thread still work via pthreads; omitting
// priority/name tweaks does not affect correctness, only optional diagnostics.

std::string getCurrentThreadName() { return "emscripten"; }

void setCurrentThreadName(const std::string&) {}

void makeThreadLowPriority() {}

void setCurrentThreadPriority(double) {}

void attachThread() {}

void detachThread() {}

}  // namespace platform
}  // namespace mbgl
