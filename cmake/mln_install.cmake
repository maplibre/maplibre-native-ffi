include(GNUInstallDirs)

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

  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)
  set(MLN_FFI_PKG_CONFIG_CFLAGS "")
  set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS "")
  set(MLN_FFI_PKG_CONFIG_LIBS "")
  if(MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "STATIC_LIBRARY")
    set(MLN_FFI_PKG_CONFIG_CFLAGS " -DMLN_STATIC")
  endif()
  if(UNIX AND MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS " -Wl,-rpath,\${libdir}")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(MLN_FFI_PKG_CONFIG_LIBS "-ldl")
  endif()

  set(pc_file "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c.pc")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/maplibre-native-c.pc.in" "${pc_file}"
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
    FILES "${pc_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()
