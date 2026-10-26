# Builds the Windows targets with clang-cl against the MSVC runtime and the
# Windows SDK.
#
# A Windows host takes both from Visual Studio, whose environment
# .mise/bin/windows-msvc-env.sh loads, and compiles with the LLVM installer's
# clang-cl. Any other host cross-compiles against the /winsysroot tree that
# MLN_FFI_WINSYSROOT names, which `mise -E msvc install` lays out with xwin, and
# runs its test binaries under Wine.

if(CMAKE_HOST_WIN32)
  set(CMAKE_C_COMPILER "$ENV{ProgramFiles}/LLVM/bin/clang-cl.exe")
  set(CMAKE_CXX_COMPILER "$ENV{ProgramFiles}/LLVM/bin/clang-cl.exe")
  return()
endif()

set(MLN_FFI_WINSYSROOT "$ENV{MLN_FFI_WINSYSROOT}"
    CACHE PATH "MSVC runtime and Windows SDK in the clang-cl /winsysroot layout")
if(NOT IS_DIRECTORY "${MLN_FFI_WINSYSROOT}/VC/Tools/MSVC")
  message(
    FATAL_ERROR
      "Cross-compiling for Windows needs MLN_FFI_WINSYSROOT to name a /winsysroot tree; install one with `mise -E msvc install`")
endif()

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES MLN_FFI_TARGET_ARCHITECTURE
     MLN_FFI_WINSYSROOT)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)
if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "arm64")
  set(CMAKE_SYSTEM_PROCESSOR ARM64)
else()
  set(CMAKE_SYSTEM_PROCESSOR AMD64)
endif()

find_program(CMAKE_C_COMPILER NAMES clang-cl REQUIRED)
set(CMAKE_CXX_COMPILER "${CMAKE_C_COMPILER}")
find_program(CMAKE_LINKER NAMES lld-link REQUIRED)
find_program(CMAKE_AR NAMES llvm-lib REQUIRED)
find_program(CMAKE_RC_COMPILER NAMES llvm-rc REQUIRED)
find_program(CMAKE_MT NAMES llvm-mt REQUIRED)

# CMake has no variable for a Windows sysroot, so pass it to every compile and
# link. lld-link spells the option with a colon, as link.exe would.
foreach(language C CXX)
  set(CMAKE_${language}_FLAGS_INIT "\"/winsysroot${MLN_FFI_WINSYSROOT}\"")
endforeach()
foreach(kind EXE SHARED MODULE)
  set(CMAKE_${kind}_LINKER_FLAGS_INIT "\"/winsysroot:${MLN_FFI_WINSYSROOT}\"")
endforeach()

# The host's own libraries and headers are never Windows ones.
set(CMAKE_FIND_ROOT_PATH "${MLN_FFI_WINSYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
