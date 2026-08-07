#pragma once

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include <array>
#include <cstddef>

#include <EGL/egl.h>
#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif
#if defined(__APPLE__)
#include <cstdint>
#include <cstring>

#include <mach-o/dyld.h>
#endif

namespace mln::core::opengl {

#if defined(__APPLE__)
// Returns the image the host already loaded. dlopen resolves a leaf name
// against the dyld search paths rather than against loaded images, so it can
// open a second copy, and handles minted by one copy are opaque pointers the
// other does not own. Reopening the path dyld resolved returns the image
// itself, including one opened RTLD_LOCAL that a process-wide symbol lookup
// would miss. The handle stays open, so the image outlives the host's own.
inline auto open_loaded_client_library(const char* library) -> void* {
  const auto count = _dyld_image_count();
  for (auto index = std::uint32_t{}; index < count; ++index) {
    const auto* path = _dyld_get_image_name(index);
    if (path == nullptr) {
      continue;
    }
    const auto* separator = std::strrchr(path, '/');
    const auto* name = separator != nullptr ? separator + 1 : path;
    if (std::strcmp(name, library) != 0) {
      continue;
    }
    if (
      auto* handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL); handle != nullptr
    ) {
      return handle;
    }
  }
  return nullptr;
}
#endif

#if defined(__linux__) || defined(__APPLE__)
// Linux binds a leaf soname to the image already loaded, so it needs no search.
inline auto open_egl_client_library(const char* library) -> void* {
#if defined(__APPLE__)
  if (auto* loaded = open_loaded_client_library(library); loaded != nullptr) {
    return loaded;
  }
#endif
  return dlopen(library, RTLD_LAZY | RTLD_LOCAL);
}

template <std::size_t N>
inline auto open_egl_client_libraries(
  const std::array<const char*, N>& libraries
) -> std::array<void*, N> {
  auto handles = std::array<void*, N>{};
  for (auto index = std::size_t{}; index < libraries.size(); ++index) {
    handles[index] = open_egl_client_library(libraries[index]);
  }
  return handles;
}

template <std::size_t N>
inline auto find_egl_client_symbol_in_handles(
  const char* name, const std::array<void*, N>& handles
) -> void* {
  for (auto* handle : handles) {
    if (handle == nullptr) {
      continue;
    }
    if (auto* proc = dlsym(handle, name); proc != nullptr) {
      return proc;
    }
  }
  return nullptr;
}

#if defined(__APPLE__)
inline auto gles_client_library_handles() -> const std::array<void*, 1>& {
  static const auto handles =
    open_egl_client_libraries(std::array<const char*, 1>{"libGLESv2.dylib"});
  return handles;
}

// ANGLE implements GLES alone, and OpenGL.framework has no EGL binding, so
// EGL_OPENGL_API has nothing to open.
inline auto gl_client_library_handles() -> const std::array<void*, 0>& {
  static const auto handles = std::array<void*, 0>{};
  return handles;
}
#else
inline auto gles_client_library_handles() -> const std::array<void*, 2>& {
  static const auto handles = open_egl_client_libraries(
    std::array<const char*, 2>{"libGLESv2.so.2", "libGLESv2.so"}
  );
  return handles;
}

inline auto gl_client_library_handles() -> const std::array<void*, 4>& {
  static const auto handles = open_egl_client_libraries(
    std::array<const char*, 4>{
      "libOpenGL.so.0", "libOpenGL.so", "libGL.so.1", "libGL.so"
    }
  );
  return handles;
}
#endif

inline auto get_egl_client_library_proc_address(const char* name, EGLenum api)
  -> void* {
  if (name == nullptr) {
    return nullptr;
  }
  if (api == EGL_OPENGL_ES_API) {
    return find_egl_client_symbol_in_handles(
      name, gles_client_library_handles()
    );
  }
  if (api == EGL_OPENGL_API) {
    return find_egl_client_symbol_in_handles(name, gl_client_library_handles());
  }
  if (
    auto* proc =
      find_egl_client_symbol_in_handles(name, gles_client_library_handles());
    proc != nullptr
  ) {
    return proc;
  }
  return find_egl_client_symbol_in_handles(name, gl_client_library_handles());
}
#else
inline auto get_egl_client_library_proc_address(const char* name, EGLenum api)
  -> void* {
  (void)name;
  (void)api;
  return nullptr;
}
#endif

}  // namespace mln::core::opengl
#endif
