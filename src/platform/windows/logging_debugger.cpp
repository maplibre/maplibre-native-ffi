#include <cstdio>
#include <string>

#include <mbgl/util/enum.hpp>
#include <mbgl/util/logging.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace mln {

// Console hosts read stderr, and GUI hosts have no console, so each record
// also goes to the debugger output stream that Visual Studio and DebugView
// show.
void Log::platformRecord(EventSeverity severity, const std::string& msg) {
  const auto message = std::string("[") +
                       Enum<EventSeverity>::toString(severity) + "] " + msg +
                       "\n";
  std::fputs(message.c_str(), stderr);
  OutputDebugStringA(message.c_str());
}

}  // namespace mln
