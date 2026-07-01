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

function(mln_install_local_opengl_runtime_libraries)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    return()
  endif()
  if(NOT MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    return()
  endif()
  if(NOT MLN_FFI_EGL_ROOT)
    return()
  endif()

  find_library(
    MLN_FFI_LOCAL_RUNTIME_EGL_LIBRARY
    NAMES EGL
    HINTS "${MLN_FFI_EGL_ROOT}" PATH_SUFFIXES . lib
    REQUIRED NO_DEFAULT_PATH)
  find_library(
    MLN_FFI_LOCAL_RUNTIME_GLESV2_LIBRARY
    NAMES GLESv2
    HINTS "${MLN_FFI_EGL_ROOT}" PATH_SUFFIXES . lib
    REQUIRED NO_DEFAULT_PATH)

  install(
    FILES "${MLN_FFI_LOCAL_RUNTIME_EGL_LIBRARY}"
    "${MLN_FFI_LOCAL_RUNTIME_GLESV2_LIBRARY}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT "${MLN_FFI_LOCAL_RUNTIME_COMPONENT}")
endfunction()

function(mln_install_local_vulkan_runtime_libraries)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    return()
  endif()
  if(NOT MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    return()
  endif()

  find_library(
    MLN_FFI_LOCAL_RUNTIME_VULKAN_LIBRARY
    NAMES vulkan.1 vulkan
    HINTS "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}"
    REQUIRED NO_DEFAULT_PATH)

  install(
    FILES "${MLN_FFI_LOCAL_RUNTIME_VULKAN_LIBRARY}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT "${MLN_FFI_LOCAL_RUNTIME_COMPONENT}")
endfunction()

function(mln_install_local_runtime_libraries)
  mln_install_local_opengl_runtime_libraries()
  mln_install_local_vulkan_runtime_libraries()
endfunction()

function(mln_install_c_api_library target)
  set(MLN_FFI_NATIVE_COMPONENT native)
  set(MLN_FFI_LOCAL_RUNTIME_COMPONENT local-runtime)

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

  if(MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    mln_install_local_runtime_libraries()
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
