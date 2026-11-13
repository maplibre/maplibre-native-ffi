function(mln_ffi_configure_options)
  set(MLN_FFI_RENDER_BACKEND ""
      CACHE STRING "Render backend that this preset tests and describes")
  set_property(
    CACHE MLN_FFI_RENDER_BACKEND
    PROPERTY STRINGS metal opengl vulkan webgpu)
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER ""
      CACHE STRING "OpenGL context provider for this preset")
  set_property(
    CACHE MLN_FFI_OPENGL_CONTEXT_PROVIDER
    PROPERTY STRINGS egl wgl webgl)
  set(MLN_FFI_EGL_ROOT "" CACHE PATH "Path to a local EGL/GLES package")
  set(MLN_FFI_DEFAULT_RENDER_BACKEND ""
      CACHE STRING "Render backend that bindings take for this target")
  set_property(
    CACHE MLN_FFI_DEFAULT_RENDER_BACKEND
    PROPERTY STRINGS metal opengl vulkan webgpu)
  set(MLN_FFI_RUST_TARGET "" CACHE STRING "Rust target triple for this build")
  set(MLN_FFI_TARGET_PLATFORM ""
      CACHE STRING "Platform name that bindings match this artifact by")
  set(MLN_FFI_ZIG_TARGET "" CACHE STRING "Zig target triple for this build")
  set(required MLN_FFI_TARGET_PLATFORM MLN_FFI_RUST_TARGET)
  # Zig can't build for the browser package, which is no library to link.
  if(NOT MLN_FFI_WEB)
    list(APPEND required MLN_FFI_ZIG_TARGET)
  endif()
  foreach(variable IN LISTS required)
    if(NOT ${variable})
      message(FATAL_ERROR "The build needs ${variable}, which the preset sets")
    endif()
  endforeach()

  string(TOLOWER "${MLN_FFI_RENDER_BACKEND}" MLN_FFI_RENDER_BACKEND)
  string(TOLOWER "${MLN_FFI_OPENGL_CONTEXT_PROVIDER}"
         MLN_FFI_OPENGL_CONTEXT_PROVIDER)
  if(NOT MLN_FFI_RENDER_BACKEND MATCHES "^(metal|opengl|vulkan|webgpu)$")
    message(FATAL_ERROR "Unsupported render backend: ${MLN_FFI_RENDER_BACKEND}")
  endif()

  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal" AND NOT APPLE)
    message(FATAL_ERROR "Metal builds require an Apple platform")
  endif()
  # mln-rs draws with Metal alone on Apple platforms.
  if(APPLE AND NOT MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    message(FATAL_ERROR "Apple builds use the Metal backend")
  endif()
  # The browser's WebGPU is the only one that a host hands the library a
  # device of: native hosts drive Vulkan, Metal, or OpenGL.
  if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu" AND NOT MLN_FFI_WEB)
    message(FATAL_ERROR "WebGPU builds are web presets")
  endif()
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    if(NOT MLN_FFI_OPENGL_CONTEXT_PROVIDER MATCHES "^(egl|wgl|webgl)$")
      message(
        FATAL_ERROR
          "Unsupported OpenGL context provider: ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}"
      )
    endif()
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl" AND NOT WIN32)
      message(FATAL_ERROR "OpenGL WGL builds require Windows")
    endif()
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl" AND NOT MLN_FFI_WEB)
      message(FATAL_ERROR "OpenGL WebGL builds are web presets")
    endif()
    if(MLN_FFI_WEB AND NOT MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
      message(
        FATAL_ERROR "Browser OpenGL builds require the webgl context provider")
    endif()
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER)
    message(
      FATAL_ERROR
        "MLN_FFI_OPENGL_CONTEXT_PROVIDER is only valid for OpenGL builds")
  endif()

  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    message(
      STATUS
        "Configuring maplibre-native-c for ${MLN_FFI_RUST_TARGET}, testing ${MLN_FFI_RENDER_BACKEND} with ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}"
    )
  else()
    message(
      STATUS
        "Configuring maplibre-native-c for ${MLN_FFI_RUST_TARGET}, testing ${MLN_FFI_RENDER_BACKEND}"
    )
  endif()

  set(MLN_FFI_RENDER_BACKEND "${MLN_FFI_RENDER_BACKEND}" PARENT_SCOPE)
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER "${MLN_FFI_OPENGL_CONTEXT_PROVIDER}"
      PARENT_SCOPE)
  set(MLN_FFI_EGL_ROOT "${MLN_FFI_EGL_ROOT}" PARENT_SCOPE)
endfunction()
