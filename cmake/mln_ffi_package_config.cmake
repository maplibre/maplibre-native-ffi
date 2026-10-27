include(CMakePackageConfigHelpers)

function(mln_ffi_install_package_config target)
  get_target_property(MLN_FFI_CMAKE_TYPE ${target} TYPE)
  set(MLN_FFI_CMAKE_STATIC_ARCHIVE "")
  if(MLN_FFI_CMAKE_TYPE STREQUAL "STATIC_LIBRARY")
    set(static_target ${target})
  elseif(TARGET ${target}_static)
    set(static_target ${target}_static)
  endif()
  if(static_target)
    get_target_property(archive ${static_target} MLN_FFI_INSTALL_ARCHIVE)
    if(archive)
      get_filename_component(MLN_FFI_CMAKE_STATIC_ARCHIVE "${archive}" NAME)
    endif()
  endif()

  set(MLN_FFI_CMAKE_LIBRARIES "")
  separate_arguments(archives NATIVE_COMMAND "${MLN_FFI_PKG_CONFIG_ARCHIVES}")
  foreach(archive IN LISTS archives)
    string(REGEX REPLACE "^-l" "" library "${archive}")
    list(APPEND MLN_FFI_CMAKE_LIBRARIES
         "\${PACKAGE_PREFIX_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${library}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endforeach()
  get_target_property(libraries mln_ffi_platform_dependencies
                      MLN_FFI_PKG_CONFIG_LIBS)
  if(libraries)
    separate_arguments(libraries NATIVE_COMMAND "${libraries}")
    list(APPEND MLN_FFI_CMAKE_LIBRARIES ${libraries})
  endif()
  set(MLN_FFI_CMAKE_STATIC_LIBRARIES ${MLN_FFI_CMAKE_LIBRARIES}
      ${MLN_FFI_STATIC_LINK_LIBRARIES})
  foreach(framework IN LISTS MLN_FFI_STATIC_LINK_FRAMEWORKS)
    list(APPEND MLN_FFI_CMAKE_STATIC_LIBRARIES "-framework ${framework}")
  endforeach()
  set(MLN_FFI_CMAKE_THREADS FALSE)
  if(TARGET Threads::Threads)
    set(MLN_FFI_CMAKE_THREADS TRUE)
    list(APPEND MLN_FFI_CMAKE_STATIC_LIBRARIES Threads::Threads)
  endif()

  set(MLN_FFI_CMAKE_LIBRARY "$<TARGET_FILE_NAME:${target}>")
  set(MLN_FFI_CMAKE_IMPLIB "")
  set(MLN_FFI_CMAKE_RUNTIME_DIR "${CMAKE_INSTALL_LIBDIR}")
  set(MLN_FFI_CMAKE_RPATH "")
  if(MLN_FFI_CMAKE_TYPE STREQUAL "STATIC_LIBRARY")
    set(MLN_FFI_CMAKE_LIBRARY "${MLN_FFI_CMAKE_STATIC_ARCHIVE}")
  elseif(WIN32)
    set(MLN_FFI_CMAKE_RUNTIME_DIR "${CMAKE_INSTALL_BINDIR}")
    set(MLN_FFI_CMAKE_IMPLIB "$<TARGET_LINKER_FILE_NAME:${target}>")
  elseif(UNIX)
    set(MLN_FFI_CMAKE_RPATH
        "LINKER:-rpath,\${PACKAGE_PREFIX_DIR}/${CMAKE_INSTALL_LIBDIR}")
  endif()
  string(REPLACE "_LIBRARY" "" MLN_FFI_CMAKE_TYPE "${MLN_FFI_CMAKE_TYPE}")

  set(config_dir "${CMAKE_INSTALL_LIBDIR}/cmake/maplibre-native-c")
  set(config "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c-config.cmake")
  configure_package_config_file(
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/maplibre-native-c-config.cmake.in"
    "${config}.in"
    INSTALL_DESTINATION "${config_dir}"
    PATH_VARS
      CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR MLN_FFI_CMAKE_RUNTIME_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)
  file(GENERATE OUTPUT "${config}" INPUT "${config}.in")
  install(
    FILES "${config}"
    DESTINATION "${config_dir}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()
