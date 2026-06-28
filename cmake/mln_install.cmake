include(GNUInstallDirs)

function(mln_install_c_api_library target)
  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)
  set(MLN_FFI_NATIVE_COMPONENT native)
  set(MLN_FFI_LOCAL_RUNTIME_COMPONENT local-runtime)

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
    RUNTIME_DEPENDENCY_SET mln_ffi_local_runtime_dependencies
    RUNTIME
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    LIBRARY
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    ARCHIVE
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")

  set(MLN_FFI_LOCAL_RUNTIME_DEPENDENCY_DIRS
      "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}")
  if(MLN_FFI_EGL_ROOT)
    list(APPEND MLN_FFI_LOCAL_RUNTIME_DEPENDENCY_DIRS "${MLN_FFI_EGL_ROOT}"
         "${MLN_FFI_EGL_ROOT}/lib")
  endif()

  if(CMAKE_SYSTEM_NAME MATCHES "^(Darwin|Linux|Windows)$"
     AND MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    install(
      RUNTIME_DEPENDENCY_SET mln_ffi_local_runtime_dependencies
      PRE_EXCLUDE_REGEXES "^/System/" "^/usr/lib/" "^api-ms-" "^ext-ms-"
      POST_EXCLUDE_REGEXES "^/System/" "^/usr/lib/"
      DIRECTORIES ${MLN_FFI_LOCAL_RUNTIME_DEPENDENCY_DIRS}
      LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT "${MLN_FFI_LOCAL_RUNTIME_COMPONENT}"
      RUNTIME
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT "${MLN_FFI_LOCAL_RUNTIME_COMPONENT}"
      FRAMEWORK
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT "${MLN_FFI_LOCAL_RUNTIME_COMPONENT}")
  endif()

  if(
    CMAKE_SYSTEM_NAME
    STREQUAL
    "iOS"
    AND
    NOT
    MLN_FFI_IS_IOS_SIMULATOR
    AND
    MLN_FFI_C_API_LIBRARY_TYPE
    STREQUAL
    "STATIC_LIBRARY")
    foreach(
      static_dependency
      mbgl-core
      mbgl-freetype
      mbgl-harfbuzz
      mbgl-vendor-csscolorparser
      mbgl-vendor-nunicode
      mbgl-vendor-parsedate
      mbgl-vendor-sqlite
      mlt-cpp)
      if(TARGET ${static_dependency})
        install(
          TARGETS ${static_dependency}
          ARCHIVE
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
      endif()
    endforeach()
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
