# Builds the Linux and FreeBSD targets with `zig cc` / `zig c++` against the
# selected libc and LLVM libc++, rather than the build host's GCC, libc, and
# libstdc++.
#
# This sets the libc floor from the toolchain instead of from whichever image
# the build runs on, and leaves the artifacts free of any libstdc++ ABI
# requirement. Kotlin/Native's bundled Linux sysroot is the tightest consumer:
# it supplies glibc 2.19 and GCC 8.3, and it statically links its own libstdc++
# into every consumer binary.
#
# A same-architecture glibc preset is a native build, so CMAKE_SYSTEM_NAME stays
# unset and try_run, find_package, and test binaries work on the host. Musl and
# FreeBSD are always cross builds, and run their test binaries inside Alpine
# and a FreeBSD virtual machine. The RISC-V and LoongArch presets cross-compile
# from any host.

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "cmake/toolchains/zig.cmake targets Linux hosts")
endif()

set(MLN_FFI_ZIG_OS "linux"
    CACHE STRING "Target operating system (linux or freebsd)")
set_property(CACHE MLN_FFI_ZIG_OS PROPERTY STRINGS linux freebsd)
if(NOT MLN_FFI_ZIG_OS MATCHES "^(linux|freebsd)$")
  message(
    FATAL_ERROR "Unsupported Zig target operating system: ${MLN_FFI_ZIG_OS}")
endif()
set(MLN_FFI_ZIG_LIBC "gnu"
    CACHE STRING "Linux libc ABI to target (gnu or musl)")
set_property(CACHE MLN_FFI_ZIG_LIBC PROPERTY STRINGS gnu musl)
if(NOT MLN_FFI_ZIG_LIBC MATCHES "^(gnu|musl)$")
  message(FATAL_ERROR "Unsupported Linux libc ABI: ${MLN_FFI_ZIG_LIBC}")
endif()
# Zig links FreeBSD's libc from its own stubs, which start at 14.0.
set(MLN_FFI_ZIG_FREEBSD "14.0"
    CACHE STRING "Oldest FreeBSD release the FreeBSD artifacts may require")

set(MLN_FFI_ZIG_SYSROOT ""
    CACHE PATH "Target-architecture graphics loaders for the test harness")

list(
  APPEND
  CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  MLN_FFI_TARGET_ARCHITECTURE
  MLN_FFI_ZIG_OS
  MLN_FFI_ZIG_GLIBC
  MLN_FFI_ZIG_LIBC
  MLN_FFI_ZIG_FREEBSD
  MLN_FFI_ZIG_SYSROOT)

# The glibc floor is the first release that supports the architecture.
if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "arm64")
  set(MLN_FFI_ZIG_ARCH "aarch64")
  set(MLN_FFI_ZIG_GLIBC_FLOOR "2.17")
elseif(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "x64")
  set(MLN_FFI_ZIG_ARCH "x86_64")
  set(MLN_FFI_ZIG_GLIBC_FLOOR "2.17")
elseif(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "riscv64")
  set(MLN_FFI_ZIG_ARCH "riscv64")
  set(MLN_FFI_ZIG_GLIBC_FLOOR "2.27")
elseif(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "loongarch64")
  set(MLN_FFI_ZIG_ARCH "loongarch64")
  set(MLN_FFI_ZIG_GLIBC_FLOOR "2.36")
else()
  message(
    FATAL_ERROR
      "Unsupported Zig target architecture: ${MLN_FFI_TARGET_ARCHITECTURE}")
endif()
set(MLN_FFI_ZIG_GLIBC "${MLN_FFI_ZIG_GLIBC_FLOOR}"
    CACHE STRING "Oldest glibc release the Linux artifacts may require")

if(MLN_FFI_ZIG_OS STREQUAL "freebsd")
  set(MLN_FFI_ZIG_TRIPLE
      "${MLN_FFI_ZIG_ARCH}-freebsd.${MLN_FFI_ZIG_FREEBSD}-none")
elseif(MLN_FFI_ZIG_LIBC STREQUAL "musl")
  set(MLN_FFI_ZIG_TRIPLE "${MLN_FFI_ZIG_ARCH}-linux-musl")
else()
  set(MLN_FFI_ZIG_TRIPLE "${MLN_FFI_ZIG_ARCH}-linux-gnu.${MLN_FFI_ZIG_GLIBC}")
endif()

# A musl target is a cross build even when its architecture matches a glibc
# host, as a FreeBSD target always is. Marking it as one keeps configure-time
# probes from executing binaries through the wrong dynamic loader.
if(MLN_FFI_ZIG_OS STREQUAL "freebsd")
  set(CMAKE_SYSTEM_NAME FreeBSD)
  set(CMAKE_SYSTEM_VERSION "${MLN_FFI_ZIG_FREEBSD}")
  set(CMAKE_SYSTEM_PROCESSOR "${MLN_FFI_ZIG_ARCH}")
elseif(MLN_FFI_ZIG_LIBC STREQUAL "musl"
       OR NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "${MLN_FFI_ZIG_ARCH}")
  set(CMAKE_SYSTEM_NAME Linux)
  set(CMAKE_SYSTEM_PROCESSOR "${MLN_FFI_ZIG_ARCH}")
endif()

find_program(MLN_FFI_ZIG NAMES zig REQUIRED)

# Generate one wrapper per driver mode. The toolchain file is re-read for every
# try_compile, so each scratch tree gets its own copy alongside the main one.
set(MLN_FFI_ZIG_SHIM_DIR "${CMAKE_BINARY_DIR}/zig-shim")
set(MLN_FFI_SHIM_PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)
foreach(MLN_FFI_ZIG_MODE cc c++)
  set(MLN_FFI_ZIG_SHIM "${MLN_FFI_ZIG_SHIM_DIR}/zig-${MLN_FFI_ZIG_MODE}")
  configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/zig-compiler.in" "${MLN_FFI_ZIG_SHIM}"
    @ONLY)
  file(CHMOD "${MLN_FFI_ZIG_SHIM}" PERMISSIONS ${MLN_FFI_SHIM_PERMISSIONS})
endforeach()

set(CMAKE_C_COMPILER "${MLN_FFI_ZIG_SHIM_DIR}/zig-cc")
set(CMAKE_CXX_COMPILER "${MLN_FFI_ZIG_SHIM_DIR}/zig-c++")

# zig ships no standalone ar/ranlib/nm/objcopy, and CMake's clang heuristics
# look for llvm-* beside the compiler. The host's binutils know only its own
# architecture, so these come from the Rust toolchain that every Linux build
# already needs for the platform crate: its llvm-tools component and its lld.
find_program(MLN_FFI_RUSTC NAMES rustc REQUIRED)
execute_process(
  COMMAND "${MLN_FFI_RUSTC}" --print sysroot
  OUTPUT_VARIABLE MLN_FFI_RUST_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(
  COMMAND "${MLN_FFI_RUSTC}" --print host-tuple
  OUTPUT_VARIABLE MLN_FFI_RUST_HOST OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
set(MLN_FFI_LLVM_TOOLS
    "${MLN_FFI_RUST_SYSROOT}/lib/rustlib/${MLN_FFI_RUST_HOST}/bin")
foreach(MLN_FFI_LLVM_TOOL ar nm objcopy objdump strip)
  if(NOT EXISTS "${MLN_FFI_LLVM_TOOLS}/llvm-${MLN_FFI_LLVM_TOOL}")
    message(
      FATAL_ERROR
        "The Rust toolchain has no llvm-${MLN_FFI_LLVM_TOOL}; add its llvm-tools component"
    )
  endif()
  string(TOUPPER "${MLN_FFI_LLVM_TOOL}" MLN_FFI_LLVM_VARIABLE)
  set(CMAKE_${MLN_FFI_LLVM_VARIABLE}
      "${MLN_FFI_LLVM_TOOLS}/llvm-${MLN_FFI_LLVM_TOOL}"
      CACHE FILEPATH "" FORCE)
endforeach()
set(CMAKE_LINKER "${MLN_FFI_LLVM_TOOLS}/gcc-ld/ld.lld" CACHE FILEPATH "" FORCE)
# llvm-tools installs no llvm-ranlib, whose work is llvm-ar's `s` operation.
set(MLN_FFI_RANLIB_SHIM "${MLN_FFI_ZIG_SHIM_DIR}/ranlib")
file(WRITE "${MLN_FFI_RANLIB_SHIM}" "#!/bin/sh\nexec '${CMAKE_AR}' s \"$@\"\n")
file(CHMOD "${MLN_FFI_RANLIB_SHIM}" PERMISSIONS ${MLN_FFI_SHIM_PERMISSIONS})
set(CMAKE_RANLIB "${MLN_FFI_RANLIB_SHIM}" CACHE FILEPATH "" FORCE)

# zig reports no implicit link directories, so CMake leaves
# CMAKE_LIBRARY_ARCHITECTURE empty and find_library never reaches the Debian
# multiarch directory that holds the Vulkan and EGL loaders.
set(CMAKE_LIBRARY_ARCHITECTURE "${MLN_FFI_ZIG_ARCH}-linux-gnu")

# A host of another architecture has no loaders for the target, so a target
# architecture that the CI runners lack brings its own.
if(MLN_FFI_ZIG_SYSROOT)
  execute_process(
    COMMAND
      "${CMAKE_CURRENT_LIST_DIR}/../../tools/deps/sync-linux-sysroot"
      "${MLN_FFI_TARGET_ARCHITECTURE}" "${MLN_FFI_ZIG_SYSROOT}"
    COMMAND_ERROR_IS_FATAL ANY)
  set(CMAKE_FIND_ROOT_PATH "${MLN_FFI_ZIG_SYSROOT}")
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  # The loaders stand in for the target's system libraries, so the test
  # executables find the target's own at run time rather than these.
  list(APPEND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
       "${MLN_FFI_ZIG_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

# zig builds libc++, libc++abi, and libunwind from source the first time a
# target needs them, and those vendored headers carry nullability annotations
# the rest of the tree does not use.
set(MLN_FFI_ZIG_DIAGNOSTIC_FLAGS
    "-Wno-nullability-completeness -Wno-unused-command-line-argument")
set(CMAKE_C_FLAGS_INIT "${MLN_FFI_ZIG_DIAGNOSTIC_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${MLN_FFI_ZIG_DIAGNOSTIC_FLAGS}")

# zig links libc++ into every binary it produces, so the platform layer bundles
# that runtime into the distributed archive rather than leaving it to consumers.
set(MLN_FFI_CXX_RUNTIME_IS_BUNDLED TRUE)
