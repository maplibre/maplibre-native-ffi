#include <string>

#include <mln/util/enum.hpp>
#include <mln/util/logging.hpp>

#include <android/log.h>

namespace mln {
namespace {

constexpr char kMapLibreLogcatTag[] = "MapLibreNative";

android_LogPriority logPriorityForSeverity(EventSeverity severity) {
  switch (severity) {
    case EventSeverity::Debug:
      return ANDROID_LOG_DEBUG;
    case EventSeverity::Info:
      return ANDROID_LOG_INFO;
    case EventSeverity::Warning:
      return ANDROID_LOG_WARN;
    case EventSeverity::Error:
      return ANDROID_LOG_ERROR;
    case EventSeverity::SeverityCount:
      break;
  }
  return ANDROID_LOG_INFO;
}

}  // namespace

void Log::platformRecord(EventSeverity severity, const std::string& msg) {
  const auto message =
    std::string("[") + Enum<EventSeverity>::toString(severity) + "] " + msg;
  __android_log_write(
    logPriorityForSeverity(severity), kMapLibreLogcatTag, message.c_str()
  );
}

}  // namespace mln
