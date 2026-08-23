# The toolchain builds against its selected libc and links its own static C++
# runtime, rather than taking either from the build host. The glibc target uses
# an old ABI floor, and the musl target stays independent of a host sysroot.
# That is what lets each distributed archive stand on its own.
#
# See cmake/toolchains/zig-linux.cmake and the Kotlin publishing doc.
function(mln_ffi_configure_linux_archive_contents target)
  if(NOT MLN_FFI_CXX_RUNTIME_IS_BUNDLED)
    return()
  endif()

  # The compiler builds its runtime on demand into content-addressed cache
  # directories, and the paths differ per optimization level, so ask a release
  # link which ones it resolves rather than guessing the layout.
  set(probe_source "${CMAKE_CURRENT_BINARY_DIR}/mln-cxx-runtime-probe.cpp")
  file(WRITE "${probe_source}"
       "#include <stdexcept>\nint main() { try { throw std::runtime_error(\"\"); } catch (...) { return 1; } }\n"
  )
  separate_arguments(probe_flags NATIVE_COMMAND "${CMAKE_CXX_FLAGS_RELEASE}")
  execute_process(
    COMMAND
      "${CMAKE_CXX_COMPILER}"
      ${probe_flags}
      "${probe_source}"
      -o
      "${CMAKE_CURRENT_BINARY_DIR}/mln-cxx-runtime-probe"
      -v
      OUTPUT_QUIET
    ERROR_VARIABLE probe_log
    RESULT_VARIABLE probe_result)
  if(NOT probe_result EQUAL 0)
    message(FATAL_ERROR "Could not probe the C++ runtime:\n${probe_log}")
  endif()
  string(
    REGEX MATCHALL
    "[^ \t\r\n]+/lib(c\\+\\+abi|c\\+\\+|unwind|compiler_rt|c_nonshared)\\.a"
    runtime_archives "${probe_log}")
  list(REMOVE_DUPLICATES runtime_archives)
  if(NOT runtime_archives)
    message(
      FATAL_ERROR
        "The toolchain reported no C++ runtime to bundle:\n${probe_log}")
  endif()

  # The bundled runtime is redistributed inside every Linux artifact, so its
  # notices ship with them. compiler-rt carries no separate notice in the
  # toolchain tree; it is part of the same LLVM project as the three below.
  execute_process(
    COMMAND "${MLN_FFI_ZIG}" env
    OUTPUT_VARIABLE zig_environment
    RESULT_VARIABLE zig_environment_result)
  if(NOT zig_environment_result EQUAL 0)
    message(FATAL_ERROR "Could not read the toolchain environment")
  endif()
  if(NOT zig_environment MATCHES "\\.lib_dir = \"([^\"]+)\"")
    message(FATAL_ERROR "No library directory in:\n${zig_environment}")
  endif()
  set(zig_library_directory "${CMAKE_MATCH_1}")
  foreach(component libcxx libcxxabi libunwind)
    mln_ffi_add_license(
      ${target} "${zig_library_directory}/${component}/LICENSE.TXT"
      "llvm-${component}.txt")
  endforeach()

  # A consumer links this archive next to a C++ runtime of its own. Everything
  # but the C API entry points becomes internal to the archive, and the one
  # support symbol that has to keep a standard name is renamed, so the two
  # cannot collide. The second rename is the compiler-generated reference to the
  # first and has to move with it.
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_ARCHIVE_BUNDLED_RUNTIME
      "${runtime_archives}"
      MLN_FFI_ARCHIVE_KEEP_GLOBAL
      "mln_*;__mln_personality_v0"
      MLN_FFI_ARCHIVE_RENAME_SYMBOLS
      "__gxx_personality_v0=__mln_personality_v0;DW.ref.__gxx_personality_v0=DW.ref.__mln_personality_v0"
      # The graphics loaders the test harness links come from the build host.
      # The glibc test runs there, and the musl test resolves matching loaders
      # inside its Alpine container.
      MLN_FFI_TEST_LINK_OPTIONS
      "LINKER:--allow-shlib-undefined")
endfunction()

function(mln_ffi_configure_platform_dependencies target)
  find_package(Threads REQUIRED)

  include(FetchContent)
  set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_BENCH OFF CACHE BOOL "" FORCE)
  fetchcontent_declare(
    mln_ffi_zlib_source
    URL
      "https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz"
    URL_HASH
      "SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23"
    EXCLUDE_FROM_ALL)
  fetchcontent_declare(
    mln_ffi_libuv_source
    URL "https://dist.libuv.org/dist/v1.48.0/libuv-v1.48.0.tar.gz"
    URL_HASH
      "SHA256=7f1db8ac368d89d1baf163bac1ea5fe5120697a73910c8ae6b2fffb3551d59fb"
    EXCLUDE_FROM_ALL)
  fetchcontent_makeavailable(mln_ffi_zlib_source mln_ffi_libuv_source)
  mln_ffi_add_license(${target} "${mln_ffi_zlib_source_SOURCE_DIR}/LICENSE"
                      "zlib.txt")
  mln_ffi_add_license(${target} "${mln_ffi_libuv_source_SOURCE_DIR}/LICENSE"
                      "libuv.txt")
  mln_ffi_add_license(
    ${target} "${mln_ffi_libuv_source_SOURCE_DIR}/LICENSE-extra"
    "libuv-extra.txt")

  # `m` is here rather than private to the C API target so that it reaches a
  # static consumer, which links the archive without that target's own line.
  target_link_libraries(
    ${target}
    INTERFACE Threads::Threads zlibstatic uv_a m ${CMAKE_DL_LIBS})
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      TRUE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      TRUE
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_ARCHIVE_FORMAT
      elf
      MLN_FFI_STATIC_ARCHIVES
      "mbgl-vendor-icu;zlibstatic;uv_a"
      MLN_FFI_PKG_CONFIG_LIBS
      -ldl
      MLN_FFI_TEST_SUPPORTED
      TRUE)
  mln_ffi_configure_linux_archive_contents(${target})
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(target_architecture arm64)
    set(zig_architecture aarch64)
  else()
    set(target_architecture x64)
    set(zig_architecture x86_64)
  endif()
  if(MLN_FFI_ZIG_LIBC STREQUAL "musl")
    set(target_platform "linux-musl-${target_architecture}")
    set(zig_target "${zig_architecture}-linux-musl")
  else()
    set(target_platform "linux-${target_architecture}")
    set(zig_target "${zig_architecture}-linux-gnu")
  endif()
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_TARGET_PLATFORM "${target_platform}" MLN_FFI_ZIG_TARGET
      "${zig_target}")
endfunction()

function(mln_ffi_configure_platform target)
  include(mln_ffi_rust)

  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_LINUX_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/thread.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp)

  set(MLN_FFI_LINUX_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/rust/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_LINUX_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_LINUX_SOURCES})

  set_source_files_properties(
    ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  target_include_directories(
    ${target}
    SYSTEM
    BEFORE
    PRIVATE ${MLN_FFI_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE mbgl-vendor-icu MLN_FFI::PlatformDependencies)

  mln_ffi_link_rust_platform(${target})
endfunction()
