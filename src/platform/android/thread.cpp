#include <array>
#include <cmath>
#include <string>

#include <mbgl/platform/thread.hpp>
#include <mbgl/util/event.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/platform.hpp>

#include <sys/prctl.h>
#include <sys/resource.h>

namespace mbgl::platform {

auto getCurrentThreadName() -> std::string {
  auto name = std::array<char, 32>{"unknown"};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, misc-include-cleaner)
  if (prctl(PR_GET_NAME, name.data()) == -1) {
    Log::Warning(Event::General, "Couldn't get thread name");
  }

  return name.data();
}

void setCurrentThreadName(const std::string& name) {
  auto truncated = name.substr(0, 15);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, misc-include-cleaner)
  if (prctl(PR_SET_NAME, truncated.c_str()) == -1) {
    Log::Warning(Event::General, "Couldn't set thread name");
  }
}

void makeThreadLowPriority() {
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (setpriority(PRIO_PROCESS, 0, 19) < 0) {
    Log::Warning(Event::General, "Couldn't set thread priority");
  }
}

void setCurrentThreadPriority(double priority) {
  if (!std::isfinite(priority) || priority < -20 || priority > 19) {
    Log::Warning(Event::General, "Couldn't set thread priority");
    return;
  }

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (setpriority(PRIO_PROCESS, 0, int(priority)) < 0) {
    Log::Warning(Event::General, "Couldn't set thread priority");
  }
}

void attachThread() {}

void detachThread() {}

}  // namespace mbgl::platform
