#include <string>

#include <mbgl/util/enum.hpp>
#include <mbgl/util/logging.hpp>

#include <hilog/log.h>

namespace mbgl {
namespace {

constexpr unsigned int kMapLibreHilogDomain = 0x4d4c;
constexpr char kMapLibreHilogTag[] = "MapLibreNative";

LogLevel logLevelForSeverity(EventSeverity severity) {
  switch (severity) {
    case EventSeverity::Debug:
      return LOG_DEBUG;
    case EventSeverity::Info:
      return LOG_INFO;
    case EventSeverity::Warning:
      return LOG_WARN;
    case EventSeverity::Error:
      return LOG_ERROR;
    case EventSeverity::SeverityCount:
      break;
  }
  return LOG_INFO;
}

std::string escapeHilogFormatString(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char character : value) {
    escaped += character;
    if (character == '%') {
      escaped += '%';
    }
  }
  return escaped;
}

}  // namespace

void Log::platformRecord(EventSeverity severity, const std::string& msg) {
  const auto message =
    std::string("[") + Enum<EventSeverity>::toString(severity) + "] " + msg;
  const auto escapedMessage = escapeHilogFormatString(message);
  OH_LOG_PrintMsg(
    LOG_APP, logLevelForSeverity(severity), kMapLibreHilogDomain,
    kMapLibreHilogTag, escapedMessage.c_str()
  );
}

}  // namespace mbgl
