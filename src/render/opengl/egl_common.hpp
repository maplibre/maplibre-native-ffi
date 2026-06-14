#pragma once

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include <array>
#include <cstddef>

#include <EGL/egl.h>
#if defined(__linux__)
#include <dlfcn.h>
#endif

namespace mln::core::opengl {

#if defined(__linux__)
template <std::size_t N>
inline auto open_egl_client_libraries(
  const std::array<const char*, N>& libraries
) -> std::array<void*, N> {
  auto handles = std::array<void*, N>{};
  for (auto index = std::size_t{}; index < libraries.size(); ++index) {
    handles[index] = dlopen(libraries[index], RTLD_LAZY | RTLD_LOCAL);
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
