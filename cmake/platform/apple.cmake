include_guard(GLOBAL)

function(mln_ffi_configure_apple_toolchain_defaults)
  # Default iOS and tvOS to 15.0, the first release with Metal Shading
  # Language 2.4, before the Apple toolchain derives compiler targets from this
  # value.
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

function(mln_ffi_apple_is_embedded out_var)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS|visionOS)$" AND NOT maccatalyst)
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(mln_ffi_configure_platform_dependencies target)
  if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Darwin|iOS|tvOS|visionOS)$")
    message(FATAL_ERROR "Unsupported Apple platform: ${CMAKE_SYSTEM_NAME}")
  endif()
  set_target_properties(
    ${target}
    PROPERTIES MLN_FFI_SHARED_SUPPORTED TRUE MLN_FFI_ARCHIVE_FORMAT apple)

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
endfunction()
