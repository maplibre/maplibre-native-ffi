# CMake has no Mac Catalyst platform: the iOS system name with the macosx
# sysroot selects it, and the `macabi` compiler target carries the rest.

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  message(FATAL_ERROR "cmake/toolchains/apple.cmake targets macOS hosts")
endif()

string(TOLOWER "${CMAKE_OSX_SYSROOT}" MLN_FFI_APPLE_SYSROOT_NAME)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS"
   AND MLN_FFI_APPLE_SYSROOT_NAME MATCHES "macosx")
  set(MLN_FFI_APPLE_MACCATALYST ON)
  set(MLN_FFI_APPLE_TRIPLE
      "arm64-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}-macabi")
  foreach(MLN_FFI_APPLE_LANGUAGE C CXX OBJC OBJCXX)
    set(CMAKE_${MLN_FFI_APPLE_LANGUAGE}_COMPILER_TARGET
        "${MLN_FFI_APPLE_TRIPLE}")
  endforeach()
  # A Catalyst process in CMake's placeholder app bundle cannot reach the
  # Metal shader compiler service.
  set(CMAKE_MACOSX_BUNDLE OFF)
endif()
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_OSX_DEPLOYMENT_TARGET
     CMAKE_OSX_SYSROOT)
