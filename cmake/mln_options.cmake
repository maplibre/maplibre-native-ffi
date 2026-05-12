function(mln_configure_options)
  set(MLN_WITH_CORE_ONLY ON CACHE BOOL "Build only MapLibre Native core" FORCE)
  set(MLN_WITH_GLFW OFF
      CACHE BOOL "Disable MapLibre Native GLFW platform" FORCE)
  set(MLN_WITH_PMTILES ON
      CACHE BOOL "Build MapLibre Native PMTiles support" FORCE)

  set(MLN_FFI_RENDER_BACKEND ""
      CACHE
        STRING "Render backend for this wrapper build: metal, vulkan, or opengl")
  set_property(
    CACHE MLN_FFI_RENDER_BACKEND
    PROPERTY STRINGS metal vulkan opengl)

  if(NOT MLN_FFI_RENDER_BACKEND)
    if(APPLE)
      set(MLN_FFI_RENDER_BACKEND "metal")
    elseif(WIN32)
      set(MLN_FFI_RENDER_BACKEND "vulkan")
    elseif(ANDROID)
      set(MLN_FFI_RENDER_BACKEND "vulkan")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      set(MLN_FFI_RENDER_BACKEND "vulkan")
    endif()
  endif()

  string(TOLOWER "${MLN_FFI_RENDER_BACKEND}" MLN_FFI_RENDER_BACKEND)
  if(NOT MLN_FFI_RENDER_BACKEND MATCHES "^(metal|vulkan|opengl)$")
    message(FATAL_ERROR "Unsupported render backend: ${MLN_FFI_RENDER_BACKEND}")
  endif()
  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal" AND NOT APPLE)
    message(FATAL_ERROR "Metal builds require an Apple platform")
  endif()
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl" AND APPLE)
    message(
      FATAL_ERROR
        "OpenGL backend is not supported on Apple platforms; use metal")
  endif()
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl" AND WIN32)
    message(
      FATAL_ERROR
        "OpenGL/WGL backend is not yet supported on Windows in this build; use vulkan")
  endif()
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl" AND ANDROID)
    message(
      FATAL_ERROR
        "OpenGL/EGL backend is not yet supported on Android in this build; use vulkan")
  endif()

  set(MLN_WITH_METAL OFF CACHE BOOL "Build MapLibre Native Metal backend" FORCE)
  set(MLN_WITH_VULKAN OFF
      CACHE BOOL "Build MapLibre Native Vulkan backend" FORCE)
  set(MLN_WITH_OPENGL OFF
      CACHE BOOL "Build MapLibre Native OpenGL backend" FORCE)
  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    set(MLN_WITH_METAL ON
        CACHE BOOL "Build MapLibre Native Metal backend" FORCE)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    set(MLN_WITH_VULKAN ON
        CACHE BOOL "Build MapLibre Native Vulkan backend" FORCE)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    set(MLN_WITH_OPENGL ON
        CACHE BOOL "Build MapLibre Native OpenGL backend" FORCE)
  endif()

  set(MLN_WITH_WERROR OFF
      CACHE BOOL "Do not fail wrapper builds on MapLibre Native warnings" FORCE)

  option(MLN_FFI_ENABLE_CLANG_TIDY "Run clang-tidy for wrapper sources" ON)

  message(
    STATUS
      "Configuring maplibre-native-c ${MLN_FFI_RENDER_BACKEND} backend (platform: ${CMAKE_SYSTEM_NAME})")

  set(MLN_FFI_RENDER_BACKEND "${MLN_FFI_RENDER_BACKEND}" PARENT_SCOPE)
endfunction()
