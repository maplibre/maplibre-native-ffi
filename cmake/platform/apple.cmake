include_guard(GLOBAL)

function(mln_ffi_configure_apple_toolchain_defaults)
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

function(mln_ffi_configure_platform_dependencies target)
  target_link_libraries(
    ${target}
    INTERFACE
      "-framework CoreFoundation" "-framework CoreGraphics"
      "-framework CoreText" "-framework Foundation" "-framework ImageIO" z)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR TRUE MLN_FFI_DEFAULT_THREAD_LOCAL TRUE
      MLN_FFI_SHARED_SUPPORTED TRUE)
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(
      ${target}
      PROPERTIES
        MLN_FFI_TARGET_PLATFORM ios-arm64 MLN_FFI_ZIG_TARGET aarch64-ios)
    if(CMAKE_OSX_SYSROOT MATCHES "[iI][pP]hone[Ss]imulator")
      set_target_properties(
        ${target}
        PROPERTIES
          MLN_FFI_TARGET_PLATFORM ios-simulator-arm64 MLN_FFI_ZIG_TARGET
          aarch64-ios-simulator MLN_FFI_TEST_SUPPORTED TRUE)
    else()
      set_target_properties(${target} PROPERTIES MLN_FFI_TEST_SUPPORTED FALSE)
    endif()
  else()
    set_target_properties(
      ${target}
      PROPERTIES
        MLN_FFI_TARGET_PLATFORM macos-arm64 MLN_FFI_ZIG_TARGET aarch64-macos
        MLN_FFI_TEST_SUPPORTED TRUE)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(MLN_FFI_ZIG_LIBC_SYSROOT "${CMAKE_OSX_SYSROOT}")
    if(NOT IS_ABSOLUTE "${MLN_FFI_ZIG_LIBC_SYSROOT}")
      execute_process(
        COMMAND xcrun --sdk "${MLN_FFI_ZIG_LIBC_SYSROOT}" --show-sdk-path
        OUTPUT_VARIABLE
          MLN_FFI_ZIG_LIBC_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    endif()
    set_target_properties(
      ${target}
      PROPERTIES
        MLN_FFI_ZIG_LIBC_SYSROOT "${MLN_FFI_ZIG_LIBC_SYSROOT}"
        MLN_FFI_ZIG_LIBC_INCLUDE_DIR "${MLN_FFI_ZIG_LIBC_SYSROOT}/usr/include"
        MLN_FFI_ZIG_LIBC_CRT_DIR "")
    set(MLN_FFI_APPLE_SDK "${MLN_FFI_ZIG_LIBC_SYSROOT}")
  else()
    set(MLN_FFI_APPLE_SDK "${CMAKE_OSX_SYSROOT}")
    if(NOT MLN_FFI_APPLE_SDK)
      set(MLN_FFI_APPLE_SDK macosx)
    endif()
    if(NOT IS_ABSOLUTE "${MLN_FFI_APPLE_SDK}")
      execute_process(
        COMMAND xcrun --sdk "${MLN_FFI_APPLE_SDK}" --show-sdk-path
        OUTPUT_VARIABLE MLN_FFI_APPLE_SDK OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    endif()
  endif()

  find_program(MLN_FFI_LIBTOOL NAMES libtool REQUIRED)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_ARCHIVE_FORMAT apple MLN_FFI_ARCHIVE_TOOL "${MLN_FFI_LIBTOOL}")
endfunction()

function(mln_ffi_configure_platform target)
  set(MLN_FFI_VENDOR_APPLE_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/qt/src/mbgl/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/async_task.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/collator.mm
      ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../src/platform/apple/http_file_source.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/image.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/local_glyph_rasterizer.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/native_apple_interface.m
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/number_format.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/nsthread.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/run_loop.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/string_nsstring.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/timer.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_APPLE_SOURCES})

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/include
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/src
      ${MLN_FFI_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE mbgl-vendor-metal-cpp MLN_FFI::PlatformDependencies)

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(
      ${target}
      PROPERTIES
        BUILD_WITH_INSTALL_NAME_DIR YES INSTALL_NAME_DIR "${PROJECT_BINARY_DIR}")
  endif()
endfunction()
