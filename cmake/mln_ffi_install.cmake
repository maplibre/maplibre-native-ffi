include(GNUInstallDirs)

# Records the commit an artifact was built from, so a downloaded prefix can name
# its provenance. Set MLN_FFI_GIT_SHA to skip discovery when building outside a
# checkout. This resolves at configure time, so a build tree that predates the
# current commit keeps the older value until it is reconfigured.
function(mln_ffi_resolve_git_sha)
  if(MLN_FFI_GIT_SHA)
    return()
  endif()
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      OUTPUT_VARIABLE git_sha OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
      RESULT_VARIABLE git_result)
    if(git_result EQUAL 0 AND git_sha)
      set(MLN_FFI_GIT_SHA "${git_sha}" PARENT_SCOPE)
      return()
    endif()
  endif()
  set(MLN_FFI_GIT_SHA "unknown" PARENT_SCOPE)
endfunction()

function(mln_ffi_install_zig_libc)
  get_target_property(MLN_FFI_ZIG_LIBC_SYSROOT mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_LIBC_SYSROOT)
  if(NOT MLN_FFI_ZIG_LIBC_SYSROOT)
    return()
  endif()
  get_target_property(MLN_FFI_ZIG_LIBC_INCLUDE_DIR mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_LIBC_INCLUDE_DIR)
  get_target_property(MLN_FFI_ZIG_LIBC_CRT_DIR mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_LIBC_CRT_DIR)

  set(MLN_FFI_ZIG_LIBC_FILE "${CMAKE_CURRENT_BINARY_DIR}/zig-libc")
  file(
    WRITE
    "${MLN_FFI_ZIG_LIBC_FILE}"
    "include_dir=${MLN_FFI_ZIG_LIBC_INCLUDE_DIR}\n"
    "sys_include_dir=${MLN_FFI_ZIG_LIBC_SYSROOT}/usr/include\n"
    "crt_dir=${MLN_FFI_ZIG_LIBC_CRT_DIR}\n"
    "msvc_lib_dir=\n"
    "kernel32_lib_dir=\n"
    "gcc_dir=\n")
  install(
    FILES "${MLN_FFI_ZIG_LIBC_FILE}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

function(mln_ffi_install_c_api_complete_static_archive target)
  get_target_property(MLN_FFI_INSTALL_ARCHIVE ${target} MLN_FFI_INSTALL_ARCHIVE)
  # A platform that merges nothing leaves this unset -- see
  # mln_ffi_configure_complete_static_archive(). The rest of the prefix, headers
  # and pkg-config included, still installs; the artifact such a platform
  # distributes instead is its own task.
  if(NOT MLN_FFI_INSTALL_ARCHIVE)
    return()
  endif()
  install(
    FILES "${MLN_FFI_INSTALL_ARCHIVE}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")

endfunction()

# Installs the Rust platform library beside the archive that needs it.
#
# It stays out of the merged archive because it carries its own copy of Rust's
# `std`, and a Rust host linking both gets two: `wasm-ld` and every other linker
# reject that on the allocator shims and the panic runtime. Such a host takes
# this library as a cargo dependency and links the archive alone; every other
# host links both, which pkg-config states.
function(mln_ffi_install_platform_library)
  if(NOT TARGET mln_ffi_platform_rust)
    return()
  endif()
  install(
    FILES "$<TARGET_FILE:mln_ffi_platform_rust>"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

# Publishes what a host needs to link the module itself, and reports where the
# library it should link lives.
#
# A browser build ships archives rather than a linked module, so the host runs
# the final emcc link. The options that link needs are the ones CMake applied,
# collected by the helpers in cmake/mln_ffi_emscripten.cmake, and they travel
# two
# ways: a file for a build system that reads one flag per line, and the
# pkg-config `Libs:` line for everything else.
function(mln_ffi_install_link_options target)
  get_property(link_options GLOBAL PROPERTY MLN_FFI_EMSCRIPTEN_LINK_OPTIONS)
  if(NOT link_options)
    return()
  endif()
  list(REMOVE_DUPLICATES link_options)

  set(link_options_file "${CMAKE_CURRENT_BINARY_DIR}/emscripten-link-flags.txt")
  list(JOIN link_options "\n" link_options_lines)
  file(WRITE "${link_options_file}" "${link_options_lines}\n")
  install(
    FILES "${link_options_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")

  list(JOIN link_options " " pkg_config_link_options)
  set(MLN_FFI_PKG_CONFIG_LIBS
      "${MLN_FFI_PKG_CONFIG_LIBS} ${pkg_config_link_options}"
      PARENT_SCOPE)
endfunction()

function(mln_ffi_install_c_api_shared_target target)
  install(
    TARGETS ${target}
    RUNTIME
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    LIBRARY
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    ARCHIVE
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

function(mln_ffi_install_c_api_library target)
  set(MLN_FFI_NATIVE_COMPONENT native)
  mln_ffi_install_zig_libc()
  mln_ffi_install_licenses(${target} "${MLN_FFI_NATIVE_COMPONENT}")

  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)
  set(MLN_FFI_PKG_CONFIG_CFLAGS "")
  set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS "")
  get_target_property(MLN_FFI_PKG_CONFIG_LIBS mln_ffi_platform_dependencies
                      MLN_FFI_PKG_CONFIG_LIBS)
  if(NOT MLN_FFI_PKG_CONFIG_LIBS)
    set(MLN_FFI_PKG_CONFIG_LIBS "")
  endif()
  if(MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "STATIC_LIBRARY")
    set(MLN_FFI_PKG_CONFIG_CFLAGS " -DMLN_STATIC")
  endif()
  if(UNIX AND MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS " -Wl,-rpath,\${libdir}")
  endif()
  get_target_property(MLN_FFI_ZIG_TARGET mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_TARGET)
  get_target_property(MLN_FFI_TARGET_PLATFORM mln_ffi_platform_dependencies
                      MLN_FFI_TARGET_PLATFORM)
  mln_ffi_resolve_git_sha()
  mln_ffi_install_link_options(${target})
  mln_ffi_install_platform_library()
  # A host that links the archive links this beside it. A Rust host is the
  # exception and takes it from cargo instead; see
  # mln_ffi_install_platform_library().
  if(TARGET mln_ffi_platform_rust)
    set(MLN_FFI_PKG_CONFIG_LIBS "${MLN_FFI_PKG_CONFIG_LIBS} -lmln_ffi_platform")
  endif()

  set(pc_file "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c.pc")
  set(artifact_file
      "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c-artifact.json")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/maplibre-native-c.pc.in" "${pc_file}"
    @ONLY)
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/artifact.json.in" "${artifact_file}"
    @ONLY)

  if(MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "STATIC_LIBRARY")
    mln_ffi_install_c_api_complete_static_archive(${target})
  else()
    mln_ffi_install_c_api_shared_target(${target})
  endif()

  if(TARGET ${target}_static)
    mln_ffi_install_c_api_complete_static_archive(${target}_static)
  endif()

  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    FILES_MATCHING
    PATTERN "*.h")
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  install(
    FILES "${artifact_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    RENAME artifact.json
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  install(
    FILES "${pc_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  get_target_property(MLN_FFI_INSTALL_LIBRARY_FILES mln_ffi_render_dependencies
                      MLN_FFI_INSTALL_LIBRARY_FILES)
  if(MLN_FFI_INSTALL_LIBRARY_FILES
     AND NOT MLN_FFI_INSTALL_LIBRARY_FILES MATCHES "-NOTFOUND$")
    install(
      FILES ${MLN_FFI_INSTALL_LIBRARY_FILES}
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  endif()
  get_target_property(MLN_FFI_INSTALL_INCLUDE_DIRS mln_ffi_render_dependencies
                      MLN_FFI_INSTALL_INCLUDE_DIRS)
  if(MLN_FFI_INSTALL_INCLUDE_DIRS
     AND NOT MLN_FFI_INSTALL_INCLUDE_DIRS MATCHES "-NOTFOUND$")
    install(
      DIRECTORY ${MLN_FFI_INSTALL_INCLUDE_DIRS}
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  endif()
endfunction()
