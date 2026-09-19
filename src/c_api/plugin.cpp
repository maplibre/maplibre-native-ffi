#define MLN_BUILDING_C

#include <string>

#include "maplibre_native_c/plugin.h"

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

#if defined(_WIN32)

// LoadLibraryW takes UTF-16; plugin paths are UTF-8 on the C boundary.
std::wstring utf16FromUtf8(const std::string& utf8) {
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

std::string lastErrorMessage(const char* operation) {
  const DWORD code = GetLastError();
  char text[256] = "";
  const DWORD length = FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code,
    0, text, sizeof(text), nullptr
  );
  std::string message = operation;
  if (length > 0) {
    message += ": ";
    message += text;
    while (!message.empty() &&
           (message.back() == '\r' || message.back() == '\n')) {
      message.pop_back();
    }
  } else {
    message += " (error " + std::to_string(code) + ")";
  }
  return message;
}

#endif

using plugin_entry_point =
  mln_plugin_status (*)(mln_plugin_register_function_v1, char*, size_t);

mln_status loadPluginLibrary(
  const std::string& path, const std::string& entry_point
) {
#if defined(_WIN32)
  const std::wstring wide_path = utf16FromUtf8(path);
  if (wide_path.empty()) {
    mln::core::set_thread_error("plugin path is not valid UTF-8");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // Intentionally never FreeLibrary: registration retains the plugin's
  // callbacks for the process lifetime.
  const HMODULE library = LoadLibraryW(wide_path.c_str());
  if (library == nullptr) {
    const std::string message = lastErrorMessage("LoadLibrary failed");
    mln::core::set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  const auto entry = reinterpret_cast<plugin_entry_point>(
    reinterpret_cast<void*>(GetProcAddress(library, entry_point.c_str()))
  );
  if (entry == nullptr) {
    const std::string message = lastErrorMessage("GetProcAddress failed");
    mln::core::set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
#else
  // Intentionally never dlclose: registration retains the plugin's callbacks
  // for the process lifetime.
  void* library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (library == nullptr) {
    const char* error = dlerror();
    mln::core::set_thread_error(error != nullptr ? error : "dlopen failed");
    return MLN_STATUS_NATIVE_ERROR;
  }
  dlerror();  // Clear stale errors before the symbol lookup.
  void* symbol = dlsym(library, entry_point.c_str());
  const char* error = dlerror();
  if (error != nullptr || symbol == nullptr) {
    mln::core::set_thread_error(error != nullptr ? error : "dlsym failed");
    return MLN_STATUS_NATIVE_ERROR;
  }
  const auto entry = reinterpret_cast<plugin_entry_point>(symbol);
#endif

  char plugin_error[256] = "";
  const mln_plugin_status status =
    entry(&mln_plugin_register_v1, plugin_error, sizeof(plugin_error));
  switch (status) {
    case MLN_PLUGIN_STATUS_OK:
    case MLN_PLUGIN_STATUS_ALREADY_REGISTERED:
      return MLN_STATUS_OK;
    default:
      mln::core::set_thread_error(
        plugin_error[0] != '\0' ? plugin_error : "plugin registration failed"
      );
      return MLN_STATUS_NATIVE_ERROR;
  }
}

}  // namespace

auto mln_plugin_get_register_function_v1(void) noexcept
  -> mln_plugin_register_function_v1 {
  return &mln_plugin_register_v1;
}

mln_status mln_plugin_load_library(
  mln_buffer_view path, mln_buffer_view entry_point
) noexcept {
  return mln::c_api::status_boundary([&]() -> mln_status {
    if (path.data == nullptr || path.size == 0) {
      mln::core::set_thread_error("plugin path is empty");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (entry_point.data == nullptr || entry_point.size == 0) {
      mln::core::set_thread_error("plugin entry point is empty");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return loadPluginLibrary(
      std::string(static_cast<const char*>(path.data), path.size),
      std::string(static_cast<const char*>(entry_point.data), entry_point.size)
    );
  });
}
