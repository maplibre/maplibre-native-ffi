include(GNUInstallDirs)

function(mln_install_c_api_library target)
  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)

  set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS "")
  if(UNIX AND MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS " -Wl,-rpath,\${libdir}")
  endif()

  set(pc_file "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c.pc")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/maplibre-native-c.pc.in" "${pc_file}"
    @ONLY)

  install(
    TARGETS ${target}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING
    PATTERN "*.h")
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c")
  install(FILES "${pc_file}" DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig")
endfunction()
