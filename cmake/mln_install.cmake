include(GNUInstallDirs)

function(mln_install_zig_libc)
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

function(mln_install_c_api_complete_static_archive target)
  get_target_property(MLN_FFI_INSTALL_ARCHIVE ${target} MLN_FFI_INSTALL_ARCHIVE)
  install(
    FILES "${MLN_FFI_INSTALL_ARCHIVE}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

function(mln_install_c_api_shared_target target)
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

function(mln_install_c_api_library target)
  set(MLN_FFI_NATIVE_COMPONENT native)
  mln_install_zig_libc()

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
    mln_install_c_api_complete_static_archive(${target})
  else()
    mln_install_c_api_shared_target(${target})
  endif()

  if(TARGET ${target}_static)
    mln_install_c_api_complete_static_archive(${target}_static)
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
