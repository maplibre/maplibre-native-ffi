include_guard(GLOBAL)

function(mln_ffi_configure_apple_toolchain_defaults)
  # MapLibre Native defaults and enforces the macOS floor itself, but it leaves
  # iOS and tvOS to the caller. Default both to 15.0, the first release with
  # Metal Shading Language 2.4, before the Apple toolchain derives compiler
  # targets from this value.
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET
     AND CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS)$")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0"
        CACHE STRING "Minimum ${CMAKE_SYSTEM_NAME} deployment target" FORCE)
  elseif(NOT CMAKE_OSX_DEPLOYMENT_TARGET
         AND CMAKE_SYSTEM_NAME STREQUAL "visionOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "1.0"
        CACHE STRING "Minimum visionOS deployment target" FORCE)
  endif()
endfunction()

function(mln_ffi_apple_is_simulator out_var)
  if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

# The SDK that CMAKE_OSX_SYSROOT names, as a path. CMake may already store an
# absolute SDK path; otherwise a Mac names the SDK for xcrun to find.
function(mln_ffi_apple_sdk_path out_var)
  if(IS_ABSOLUTE "${CMAKE_OSX_SYSROOT}")
    set(${out_var} "${CMAKE_OSX_SYSROOT}" PARENT_SCOPE)
    return()
  endif()
  execute_process(
    COMMAND xcrun --sdk "${CMAKE_OSX_SYSROOT}" --show-sdk-path
    OUTPUT_VARIABLE sdk_path OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
  set(${out_var} "${sdk_path}" PARENT_SCOPE)
endfunction()

function(mln_ffi_apple_is_maccatalyst out_var)
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS"
     AND CMAKE_OSX_SYSROOT MATCHES "[Mm]ac[Oo][Ss][Xx]")
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Whether the target uses an embedded Apple SDK rather than macOS or Mac
# Catalyst.
function(mln_ffi_apple_is_embedded out_var)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS|visionOS)$" AND NOT maccatalyst)
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(mln_ffi_configure_platform_dependencies target)
  # `c++` is named rather than bundled: Apple ships libc++ as a system library
  # with a stable ABI, so the archive can leave it to the platform.
  target_link_libraries(
    ${target}
    INTERFACE
      "-framework CoreFoundation"
      "-framework CoreGraphics"
      "-framework CoreText"
      "-framework Foundation"
      "-framework ImageIO"
      z
      c++)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      FALSE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      TRUE
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_STATIC_ARCHIVES
      mbgl-vendor-icu)

  if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Darwin|iOS|tvOS|visionOS)$")
    message(FATAL_ERROR "Unsupported Apple platform: ${CMAKE_SYSTEM_NAME}")
  endif()
  # The host runs the suite for macOS, Mac Catalyst, and simulators, but not
  # for devices.
  mln_ffi_apple_is_embedded(MLN_FFI_APPLE_EMBEDDED)
  mln_ffi_apple_is_simulator(MLN_FFI_APPLE_SIMULATOR)
  if(MLN_FFI_APPLE_EMBEDDED AND NOT MLN_FFI_APPLE_SIMULATOR)
    set_target_properties(${target} PROPERTIES MLN_FFI_TEST_SUPPORTED FALSE)
  else()
    set_target_properties(${target} PROPERTIES MLN_FFI_TEST_SUPPORTED TRUE)
  endif()

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    mln_ffi_apple_sdk_path(MLN_FFI_ZIG_LIBC_SYSROOT)
    set_target_properties(
      ${target}
      PROPERTIES
        MLN_FFI_ZIG_LIBC_SYSROOT "${MLN_FFI_ZIG_LIBC_SYSROOT}"
        MLN_FFI_ZIG_LIBC_INCLUDE_DIR "${MLN_FFI_ZIG_LIBC_SYSROOT}/usr/include"
        MLN_FFI_ZIG_LIBC_CRT_DIR "")
  endif()

  find_program(MLN_FFI_LIBTOOL NAMES libtool REQUIRED)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_ARCHIVE_FORMAT apple MLN_FFI_ARCHIVE_TOOL "${MLN_FFI_LIBTOOL}")
endfunction()

function(mln_ffi_configure_platform target)
  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_APPLE_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/async_task.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/collator.mm
      ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../src/platform/apple/http_file_source.mm
      ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../src/platform/apple/i18n.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/image.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/local_glyph_rasterizer.mm
      ${MLN_FFI_SOURCE_DIR}/platform/darwin/core/logging_nslog.mm
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
    PRIVATE mbgl-vendor-icu mbgl-vendor-metal-cpp MLN_FFI::PlatformDependencies)

  # MapLibre Native warnings don't fail the build, but a call to an Apple API
  # newer than the deployment target must: it would fail at run time on the
  # oldest supported OS. This covers MapLibre Native's core and the platform
  # sources that this target compiles.
  set(MLN_FFI_APPLE_AVAILABILITY_ERROR
      $<$<COMPILE_LANGUAGE:C,CXX,OBJC,OBJCXX>:-Werror=unguarded-availability-new>)
  target_compile_options(
    mbgl-compiler-options
    INTERFACE ${MLN_FFI_APPLE_AVAILABILITY_ERROR})
  target_compile_options(${target} PRIVATE ${MLN_FFI_APPLE_AVAILABILITY_ERROR})

  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(
      ${target}
      PROPERTIES
        BUILD_WITH_INSTALL_NAME_DIR YES INSTALL_NAME_DIR "${PROJECT_BINARY_DIR}")
  endif()
endfunction()
