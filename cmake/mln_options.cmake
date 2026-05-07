function(mln_configure_options)
  set(MLN_WITH_CORE_ONLY ON CACHE BOOL "Build only MapLibre Native core" FORCE)
  set(MLN_WITH_GLFW OFF
      CACHE BOOL "Disable MapLibre Native GLFW platform" FORCE)
  set(MLN_WITH_PMTILES ON
      CACHE BOOL "Build MapLibre Native PMTiles support" FORCE)

  set(MLN_FFI_RENDER_BACKEND ""
      CACHE STRING "Render backend for this wrapper build: metal or vulkan")
  set_property(CACHE MLN_FFI_RENDER_BACKEND PROPERTY STRINGS metal vulkan)

  if(NOT MLN_FFI_RENDER_BACKEND)
    if(APPLE)
      set(MLN_FFI_RENDER_BACKEND "metal")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      set(MLN_FFI_RENDER_BACKEND "vulkan")
    endif()
  endif()

  string(TOLOWER "${MLN_FFI_RENDER_BACKEND}" MLN_FFI_RENDER_BACKEND)
  if(NOT MLN_FFI_RENDER_BACKEND MATCHES "^(metal|vulkan)$")
    message(FATAL_ERROR "Unsupported render backend: ${MLN_FFI_RENDER_BACKEND}")
  endif()
  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal" AND NOT APPLE)
    message(FATAL_ERROR "Metal builds require an Apple platform")
  endif()

  set(MLN_WITH_METAL OFF CACHE BOOL "Build MapLibre Native Metal backend" FORCE)
  set(MLN_WITH_VULKAN OFF
      CACHE BOOL "Build MapLibre Native Vulkan backend" FORCE)
  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    set(MLN_WITH_METAL ON
        CACHE BOOL "Build MapLibre Native Metal backend" FORCE)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    set(MLN_WITH_VULKAN ON
        CACHE BOOL "Build MapLibre Native Vulkan backend" FORCE)
  endif()

  set(MLN_WITH_WERROR OFF
      CACHE BOOL "Do not fail wrapper builds on MapLibre Native warnings" FORCE)

  option(MLN_FFI_ENABLE_CLANG_TIDY "Run clang-tidy for wrapper sources" ON)

  message(
    STATUS "Configuring maplibre-native-c ${MLN_FFI_RENDER_BACKEND} backend")

  set(MLN_FFI_RENDER_BACKEND "${MLN_FFI_RENDER_BACKEND}" PARENT_SCOPE)
endfunction()
