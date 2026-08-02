function(mln_configure_options)
  set(MLN_WITH_CORE_ONLY ON CACHE BOOL "Build only MapLibre Native core" FORCE)
  set(MLN_WITH_GLFW OFF
      CACHE BOOL "Disable MapLibre Native GLFW platform" FORCE)
  set(MLN_WITH_PMTILES ON
      CACHE BOOL "Build MapLibre Native PMTiles support" FORCE)

  set(MLN_FFI_RENDER_BACKEND ""
      CACHE STRING "Render backend for this wrapper build")
  set_property(
    CACHE MLN_FFI_RENDER_BACKEND
    PROPERTY STRINGS metal opengl vulkan webgpu)
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER ""
      CACHE STRING "OpenGL context provider for this wrapper build")
  set_property(
    CACHE MLN_FFI_OPENGL_CONTEXT_PROVIDER
    PROPERTY STRINGS egl wgl webgl)
  set(MLN_FFI_EGL_ROOT "" CACHE PATH "Path to a local EGL/GLES package")

  string(TOLOWER "${MLN_FFI_RENDER_BACKEND}" MLN_FFI_RENDER_BACKEND)
  string(TOLOWER "${MLN_FFI_OPENGL_CONTEXT_PROVIDER}"
         MLN_FFI_OPENGL_CONTEXT_PROVIDER)
  if(NOT MLN_FFI_RENDER_BACKEND MATCHES "^(metal|opengl|vulkan|webgpu)$")
    message(FATAL_ERROR "Unsupported render backend: ${MLN_FFI_RENDER_BACKEND}")
  endif()

  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal" AND NOT APPLE)
    message(FATAL_ERROR "Metal builds require an Apple platform")
  endif()
  # emdawnwebgpu forwards webgpu.h to the browser's own navigator.gpu, so a
  # WebGPU build here is a browser build by construction. Native WebGPU would
  # need Dawn or wgpu-native bootstrapped instead.
  if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu" AND NOT EMSCRIPTEN)
    message(FATAL_ERROR "WebGPU builds require the Emscripten toolchain")
  endif()
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    if(NOT MLN_FFI_OPENGL_CONTEXT_PROVIDER MATCHES "^(egl|wgl|webgl)$")
      message(
        FATAL_ERROR
          "Unsupported OpenGL context provider: ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
    endif()
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl" AND NOT WIN32)
      message(FATAL_ERROR "OpenGL WGL builds require Windows")
    endif()
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl" AND NOT EMSCRIPTEN)
      message(FATAL_ERROR "OpenGL WebGL builds require Emscripten")
    endif()
    if(EMSCRIPTEN AND NOT MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
      message(
        FATAL_ERROR "Browser OpenGL builds require the webgl context provider")
    endif()
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER)
    message(
      FATAL_ERROR
        "MLN_FFI_OPENGL_CONTEXT_PROVIDER is only valid for OpenGL builds")
  endif()

  set(MLN_WITH_METAL OFF CACHE BOOL "Build MapLibre Native Metal backend" FORCE)
  set(MLN_WITH_OPENGL OFF
      CACHE BOOL "Build MapLibre Native OpenGL backend" FORCE)
  set(MLN_WITH_VULKAN OFF
      CACHE BOOL "Build MapLibre Native Vulkan backend" FORCE)
  set(MLN_WITH_WEBGPU OFF
      CACHE BOOL "Build MapLibre Native WebGPU backend" FORCE)
  set(MLN_WEBGPU_IMPL_DAWN OFF
      CACHE BOOL "Build MapLibre Native WebGPU with Dawn" FORCE)
  set(MLN_WEBGPU_IMPL_WGPU OFF
      CACHE BOOL "Build MapLibre Native WebGPU with wgpu-native" FORCE)
  set(MLN_WEBGPU_EMDAWN OFF
      CACHE BOOL "Use the Emscripten Dawn WebGPU port" FORCE)
  set(MLN_WITH_EGL OFF CACHE BOOL "Build MapLibre Native EGL support" FORCE)
  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    set(MLN_WITH_METAL ON
        CACHE BOOL "Build MapLibre Native Metal backend" FORCE)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    set(MLN_WITH_OPENGL ON
        CACHE BOOL "Build MapLibre Native OpenGL backend" FORCE)
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
      set(MLN_WITH_EGL ON CACHE BOOL "Build MapLibre Native EGL support" FORCE)
    endif()
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    set(MLN_WITH_VULKAN ON
        CACHE BOOL "Build MapLibre Native Vulkan backend" FORCE)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
    set(MLN_WITH_WEBGPU ON
        CACHE BOOL "Build MapLibre Native WebGPU backend" FORCE)
    # emdawnwebgpu supplies Dawn's webgpu.h against the browser implementation,
    # so the Dawn path is the one that applies; upstream's vendor/dawn.cmake
    # carries the port flags on mbgl-vendor-dawn.
    set(MLN_WEBGPU_IMPL_DAWN ON
        CACHE BOOL "Build MapLibre Native WebGPU with Dawn" FORCE)
    set(MLN_WEBGPU_EMDAWN ON
        CACHE BOOL "Use the Emscripten Dawn WebGPU port" FORCE)
  endif()

  set(MLN_WITH_WERROR OFF
      CACHE BOOL "Do not fail wrapper builds on MapLibre Native warnings" FORCE)

  set(MLN_FFI_ENABLE_CLANG_TIDY_DEFAULT OFF)
  option(MLN_FFI_ENABLE_CLANG_TIDY "Run clang-tidy for wrapper sources"
         ${MLN_FFI_ENABLE_CLANG_TIDY_DEFAULT})

  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    message(
      STATUS
        "Configuring maplibre-native-c ${MLN_FFI_RENDER_BACKEND} backend with ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
  else()
    message(
      STATUS "Configuring maplibre-native-c ${MLN_FFI_RENDER_BACKEND} backend")
  endif()

  set(MLN_FFI_RENDER_BACKEND "${MLN_FFI_RENDER_BACKEND}" PARENT_SCOPE)
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER "${MLN_FFI_OPENGL_CONTEXT_PROVIDER}"
      PARENT_SCOPE)
  set(MLN_FFI_EGL_ROOT "${MLN_FFI_EGL_ROOT}" PARENT_SCOPE)
endfunction()
