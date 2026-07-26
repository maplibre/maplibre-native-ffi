if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  return()
endif()

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES MLN_FFI_XTOOL_SDK_BUNDLE
     CMAKE_OSX_DEPLOYMENT_TARGET CMAKE_OSX_SYSROOT)

set(MLN_FFI_XTOOL_SDK_BUNDLE
    "$ENV{HOME}/.swiftpm/swift-sdks/darwin.artifactbundle"
    CACHE PATH "Darwin Swift SDK artifact bundle installed by xtool")
if(NOT EXISTS "${MLN_FFI_XTOOL_SDK_BUNDLE}/toolset/bin/ld64.lld")
  message(
    FATAL_ERROR "No xtool Darwin SDK found; run `xtool.AppImage sdk install`")
endif()

string(TOLOWER "${CMAKE_OSX_SYSROOT}" MLN_FFI_XTOOL_SYSROOT_NAME)
if(MLN_FFI_XTOOL_SYSROOT_NAME MATCHES "iphonesimulator")
  set(CMAKE_SYSTEM_NAME iOS)
  set(MLN_FFI_XTOOL_TRIPLE
      "arm64-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}-simulator")
  set(MLN_FFI_XTOOL_SYSROOT
      "${MLN_FFI_XTOOL_SDK_BUNDLE}/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk")
elseif(MLN_FFI_XTOOL_SYSROOT_NAME MATCHES "iphoneos")
  set(CMAKE_SYSTEM_NAME iOS)
  set(MLN_FFI_XTOOL_TRIPLE "arm64-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}")
  set(MLN_FFI_XTOOL_SYSROOT
      "${MLN_FFI_XTOOL_SDK_BUNDLE}/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk")
elseif(MLN_FFI_XTOOL_SYSROOT_NAME MATCHES "macosx")
  set(CMAKE_SYSTEM_NAME Darwin)
  set(MLN_FFI_XTOOL_TRIPLE "arm64-apple-macosx${CMAKE_OSX_DEPLOYMENT_TARGET}")
  set(MLN_FFI_XTOOL_SYSROOT
      "${MLN_FFI_XTOOL_SDK_BUNDLE}/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
else()
  message(FATAL_ERROR "Unsupported Apple SDK: ${CMAKE_OSX_SYSROOT}")
endif()

find_program(MLN_FFI_XTOOL_CLANG NAMES clang REQUIRED)
find_program(MLN_FFI_XTOOL_CLANGXX NAMES clang++ REQUIRED)
set(CMAKE_C_COMPILER "${MLN_FFI_XTOOL_CLANG}")
set(CMAKE_CXX_COMPILER "${MLN_FFI_XTOOL_CLANGXX}")
set(CMAKE_OBJC_COMPILER "${MLN_FFI_XTOOL_CLANG}")
set(CMAKE_OBJCXX_COMPILER "${MLN_FFI_XTOOL_CLANGXX}")
foreach(MLN_FFI_XTOOL_LANGUAGE C CXX OBJC OBJCXX)
  set(CMAKE_${MLN_FFI_XTOOL_LANGUAGE}_COMPILER_TARGET "${MLN_FFI_XTOOL_TRIPLE}")
endforeach()

set(CMAKE_SYSTEM_PROCESSOR arm64)
set(CMAKE_OSX_SYSROOT "${MLN_FFI_XTOOL_SYSROOT}"
    CACHE PATH "Apple SDK root" FORCE)
set(MLN_FFI_XTOOL_LINKER_FLAGS
    "-B${MLN_FFI_XTOOL_SDK_BUNDLE}/toolset/bin -fuse-ld=lld")
foreach(MLN_FFI_XTOOL_LINKER_TYPE EXE SHARED MODULE)
  set(CMAKE_${MLN_FFI_XTOOL_LINKER_TYPE}_LINKER_FLAGS_INIT
      "${MLN_FFI_XTOOL_LINKER_FLAGS}")
endforeach()
set(MLN_FFI_LIBTOOL "${MLN_FFI_XTOOL_SDK_BUNDLE}/toolset/bin/libtool"
    CACHE FILEPATH "Apple archive tool supplied by xtool")
