function(mln_ffi_validate_platform)
  if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD)$")
    return()
  endif()

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    set(MLN_FFI_DETECTED_ARCHITECTURE x64)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(MLN_FFI_DETECTED_ARCHITECTURE arm64)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(riscv64|loongarch64)$")
    set(MLN_FFI_DETECTED_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
  else()
    message(
      FATAL_ERROR
        "Unsupported ${CMAKE_SYSTEM_NAME} architecture: ${CMAKE_SYSTEM_PROCESSOR}"
    )
  endif()
  if(NOT MLN_FFI_TARGET_ARCHITECTURE STREQUAL MLN_FFI_DETECTED_ARCHITECTURE)
    message(
      FATAL_ERROR
        "${CMAKE_SYSTEM_NAME} preset targets ${MLN_FFI_TARGET_ARCHITECTURE}, but the compiler targets ${MLN_FFI_DETECTED_ARCHITECTURE} (${CMAKE_SYSTEM_PROCESSOR})"
    )
  endif()
  if(MLN_FFI_RUST_TARGET MATCHES "^x86_64-")
    set(MLN_FFI_RUST_ARCHITECTURE x64)
  elseif(MLN_FFI_RUST_TARGET MATCHES "^aarch64-")
    set(MLN_FFI_RUST_ARCHITECTURE arm64)
  elseif(MLN_FFI_RUST_TARGET MATCHES "^riscv64[a-z]*-")
    set(MLN_FFI_RUST_ARCHITECTURE riscv64)
  elseif(MLN_FFI_RUST_TARGET MATCHES "^loongarch64-")
    set(MLN_FFI_RUST_ARCHITECTURE loongarch64)
  else()
    message(
      FATAL_ERROR
        "Unsupported ${CMAKE_SYSTEM_NAME} Rust target: ${MLN_FFI_RUST_TARGET}")
  endif()
  if(NOT MLN_FFI_RUST_ARCHITECTURE STREQUAL MLN_FFI_DETECTED_ARCHITECTURE)
    message(
      FATAL_ERROR
        "${CMAKE_SYSTEM_NAME} preset targets Rust ${MLN_FFI_RUST_TARGET}, but the compiler targets ${MLN_FFI_DETECTED_ARCHITECTURE} (${CMAKE_SYSTEM_PROCESSOR})"
    )
  endif()
endfunction()

# Describes the platform through properties on mln_ffi_platform_dependencies:
#
#   MLN_FFI_SHARED_SUPPORTED   whether the platform builds a shared library
#   MLN_FFI_ARCHIVE_FORMAT     elf, whose archive keeps only the C API global,
#                              or another format, copied as Cargo writes it
#   MLN_FFI_TEST_SUPPORTED     whether the native suites build and run
#   MLN_FFI_STATIC_LINK_LIBRARIES
#                              what an in-tree target that links the static
#                              archive links besides, where nothing else does
#
# The package's own description of what a static consumer links comes from
# rustc, at build time; see cmake/mln_ffi_package_metadata.cmake.
function(mln_ffi_select_platform)
  add_library(mln_ffi_platform_dependencies INTERFACE)
  add_library(MLN_FFI::PlatformDependencies ALIAS mln_ffi_platform_dependencies)

  if(EMSCRIPTEN)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/emscripten.cmake")
  elseif(APPLE)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/apple.cmake")
  elseif(CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD)$")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/linux.cmake")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/android.cmake")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/ohos.cmake")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/windows.cmake")
  else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
  endif()

  mln_ffi_configure_platform_dependencies(mln_ffi_platform_dependencies)
endfunction()
