include_guard(GLOBAL)

function(mln_configure_apple_toolchain_defaults)
  if(DEFINED CMAKE_SYSTEM_NAME)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Darwin|iOS|tvOS|watchOS|visionOS)$")
      return()
    endif()
  elseif(NOT CMAKE_HOST_APPLE)
    return()
  endif()

  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
      # Match MapLibre Native's vendored CMake, which currently forces this
      # value through maplibre-tile-spec even for iOS builds.
      set(CMAKE_OSX_DEPLOYMENT_TARGET "14.3"
          CACHE STRING "Minimum iOS deployment target" FORCE)
    elseif(NOT DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
      set(CMAKE_OSX_DEPLOYMENT_TARGET "14.3"
          CACHE STRING "Minimum macOS deployment target" FORCE)
    endif()
  endif()
endfunction()

function(mln_configure_apple_platform target)
  set(MLN_FFI_VENDOR_APPLE_SOURCES
      ${MLN_SOURCE_DIR}/platform/qt/src/mbgl/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/darwin/core/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/darwin/core/collator.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/http_file_source.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/image.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/local_glyph_rasterizer.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/native_apple_interface.m
      ${MLN_SOURCE_DIR}/platform/darwin/core/number_format.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/nsthread.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/darwin/core/string_nsstring.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/timer.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_APPLE_SOURCES})

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/darwin/core
      ${MLN_SOURCE_DIR}/platform/darwin/include
      ${MLN_SOURCE_DIR}/platform/darwin/src ${MLN_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE
      mbgl-vendor-metal-cpp "-framework CoreFoundation"
      "-framework CoreGraphics" "-framework CoreText" "-framework Foundation"
      "-framework ImageIO")

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(
      ${target}
      PROPERTIES
        BUILD_WITH_INSTALL_NAME_DIR YES INSTALL_NAME_DIR "${PROJECT_BINARY_DIR}")
  endif()
endfunction()
