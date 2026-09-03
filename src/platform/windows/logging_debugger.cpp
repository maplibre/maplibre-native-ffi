#include <cstdio>
#include <string>

#include <mln/util/enum.hpp>
#include <mln/util/logging.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace mln {
namespace {

// The narrow debugger entry point decodes with the active code page, which
// corrupts UTF-8 message text, so the record goes through the wide one.
std::wstring utf16FromUtf8(const std::string& utf8) {
  if (utf8.empty()) {
    return {};
  }
  const int length = MultiByteToWideChar(
    CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0
  );
  if (length <= 0) {
    return {};
  }
  std::wstring utf16(static_cast<size_t>(length), L'\0');
  MultiByteToWideChar(
    CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), utf16.data(), length
  );
  return utf16;
}

}  // namespace

// Console hosts read stderr, and GUI hosts have no console, so each record
// also goes to the debugger output stream that Visual Studio and DebugView
// show.
void Log::platformRecord(EventSeverity severity, const std::string& msg) {
  const auto message = std::string("[") +
                       Enum<EventSeverity>::toString(severity) + "] " + msg +
                       "\n";
  std::fputs(message.c_str(), stderr);
  OutputDebugStringW(utf16FromUtf8(message).c_str());
}

}  // namespace mln
