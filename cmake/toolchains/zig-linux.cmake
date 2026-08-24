# Builds the Linux targets with `zig cc` / `zig c++` against the selected libc
# and LLVM libc++, rather than the build host's GCC, libc, and libstdc++.
#
# This sets the glibc floor from the toolchain instead of from whichever image
# the build runs on, and leaves the artifacts free of any libstdc++ ABI
# requirement. Kotlin/Native's bundled Linux sysroot is the tightest consumer:
# it supplies glibc 2.19 and GCC 8.3, and it statically links its own libstdc++
# into every consumer binary. See the Kotlin publishing doc.
#
# A same-architecture glibc preset is a native build, so CMAKE_SYSTEM_NAME stays
# unset and try_run, find_package, and test binaries work on the host. Musl is
# always a cross build and runs its test binaries inside Alpine.

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "cmake/toolchains/zig-linux.cmake targets Linux hosts")
endif()

set(MLN_FFI_ZIG_GLIBC "2.17"
    CACHE STRING "Oldest glibc release the Linux artifacts may require")
set(MLN_FFI_ZIG_LIBC "gnu"
    CACHE STRING "Linux libc ABI to target (gnu or musl)")
set_property(CACHE MLN_FFI_ZIG_LIBC PROPERTY STRINGS gnu musl)
if(NOT MLN_FFI_ZIG_LIBC MATCHES "^(gnu|musl)$")
  message(FATAL_ERROR "Unsupported Linux libc ABI: ${MLN_FFI_ZIG_LIBC}")
endif()

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES MLN_FFI_TARGET_ARCHITECTURE
     MLN_FFI_ZIG_GLIBC MLN_FFI_ZIG_LIBC)

if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "arm64")
  set(MLN_FFI_ZIG_ARCH "aarch64")
else()
  set(MLN_FFI_ZIG_ARCH "x86_64")
endif()

if(MLN_FFI_ZIG_LIBC STREQUAL "musl")
  set(MLN_FFI_ZIG_TRIPLE "${MLN_FFI_ZIG_ARCH}-linux-musl")
else()
  set(MLN_FFI_ZIG_TRIPLE "${MLN_FFI_ZIG_ARCH}-linux-gnu.${MLN_FFI_ZIG_GLIBC}")
endif()

# A musl target is a cross build even when its architecture matches a glibc
# host. Marking it as one keeps configure-time probes from executing binaries
# through the wrong dynamic loader.
if(MLN_FFI_ZIG_LIBC STREQUAL "musl"
   OR NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "${MLN_FFI_ZIG_ARCH}")
  set(CMAKE_SYSTEM_NAME Linux)
  set(CMAKE_SYSTEM_PROCESSOR "${MLN_FFI_ZIG_ARCH}")
endif()

find_program(MLN_FFI_ZIG NAMES zig REQUIRED)

# Generate one wrapper per driver mode. The toolchain file is re-read for every
# try_compile, so each scratch tree gets its own copy alongside the main one.
set(MLN_FFI_ZIG_SHIM_DIR "${CMAKE_BINARY_DIR}/zig-shim")
foreach(MLN_FFI_ZIG_MODE cc c++)
  set(MLN_FFI_ZIG_SHIM "${MLN_FFI_ZIG_SHIM_DIR}/zig-${MLN_FFI_ZIG_MODE}")
  configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/zig-compiler.in" "${MLN_FFI_ZIG_SHIM}"
    @ONLY)
  file(
    CHMOD
    "${MLN_FFI_ZIG_SHIM}"
    PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)
endforeach()

set(CMAKE_C_COMPILER "${MLN_FFI_ZIG_SHIM_DIR}/zig-cc")
set(CMAKE_CXX_COMPILER "${MLN_FFI_ZIG_SHIM_DIR}/zig-c++")

# zig ships no standalone ar/ranlib/nm/objcopy, and CMake's clang heuristics
# look for llvm-* beside the compiler. These only repackage relocatable objects,
# so taking them from the host adds no glibc dependency.
find_program(MLN_FFI_HOST_AR NAMES ar REQUIRED)
find_program(MLN_FFI_HOST_RANLIB NAMES ranlib REQUIRED)
find_program(MLN_FFI_HOST_LD NAMES ld REQUIRED)
find_program(MLN_FFI_HOST_NM NAMES nm REQUIRED)
find_program(MLN_FFI_HOST_OBJCOPY NAMES objcopy REQUIRED)
find_program(MLN_FFI_HOST_OBJDUMP NAMES objdump REQUIRED)
find_program(MLN_FFI_HOST_STRIP NAMES strip REQUIRED)
set(CMAKE_AR "${MLN_FFI_HOST_AR}" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${MLN_FFI_HOST_RANLIB}" CACHE FILEPATH "" FORCE)
set(CMAKE_LINKER "${MLN_FFI_HOST_LD}" CACHE FILEPATH "" FORCE)
set(CMAKE_NM "${MLN_FFI_HOST_NM}" CACHE FILEPATH "" FORCE)
set(CMAKE_OBJCOPY "${MLN_FFI_HOST_OBJCOPY}" CACHE FILEPATH "" FORCE)
set(CMAKE_OBJDUMP "${MLN_FFI_HOST_OBJDUMP}" CACHE FILEPATH "" FORCE)
set(CMAKE_STRIP "${MLN_FFI_HOST_STRIP}" CACHE FILEPATH "" FORCE)

# zig reports no implicit link directories, so CMake leaves
# CMAKE_LIBRARY_ARCHITECTURE empty and find_library never reaches the Debian
# multiarch directory that holds the Vulkan and EGL loaders.
set(CMAKE_LIBRARY_ARCHITECTURE "${MLN_FFI_ZIG_ARCH}-linux-gnu")

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
