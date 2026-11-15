include(CMakePackageConfigHelpers)

# Writes the relocatable CMake package's config file. Its static target's link
# libraries are filled in at build time, so the config is configured to
# `out_template` here and written to `out_config` by the build; see
# cmake/mln_ffi_package_metadata.cmake.
function(mln_ffi_install_package_config target out_template out_config)
  get_target_property(shared ${target} MLN_FFI_SHARED_FILE)
  get_target_property(implib ${target} MLN_FFI_IMPLIB_FILE)
  get_target_property(static ${target} MLN_FFI_STATIC_FILE)
  get_filename_component(MLN_FFI_CMAKE_STATIC_ARCHIVE "${static}" NAME)

  set(MLN_FFI_CMAKE_STATIC_LIBRARIES "%%LINK_CMAKE%%")
  set(MLN_FFI_CMAKE_THREADS FALSE)
  if(TARGET Threads::Threads)
    set(MLN_FFI_CMAKE_THREADS TRUE)
    string(APPEND MLN_FFI_CMAKE_STATIC_LIBRARIES ";Threads::Threads")
  endif()

  set(MLN_FFI_CMAKE_IMPLIB "")
  set(MLN_FFI_CMAKE_RUNTIME_DIR "${CMAKE_INSTALL_LIBDIR}")
  set(MLN_FFI_CMAKE_RPATH "")
  if(NOT shared)
    set(MLN_FFI_CMAKE_TYPE STATIC)
    set(MLN_FFI_CMAKE_LIBRARY "${MLN_FFI_CMAKE_STATIC_ARCHIVE}")
  else()
    set(MLN_FFI_CMAKE_TYPE SHARED)
    get_filename_component(MLN_FFI_CMAKE_LIBRARY "${shared}" NAME)
    if(WIN32)
      set(MLN_FFI_CMAKE_RUNTIME_DIR "${CMAKE_INSTALL_BINDIR}")
      get_filename_component(MLN_FFI_CMAKE_IMPLIB "${implib}" NAME)
    elseif(UNIX)
      set(MLN_FFI_CMAKE_RPATH
          "LINKER:-rpath,\${PACKAGE_PREFIX_DIR}/${CMAKE_INSTALL_LIBDIR}")
    endif()
  endif()

  set(config_dir "${CMAKE_INSTALL_LIBDIR}/cmake/maplibre-native-c")
  set(config "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c-config.cmake")
  configure_package_config_file(
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/maplibre-native-c-config.cmake.in"
    "${config}.in"
    INSTALL_DESTINATION "${config_dir}"
    PATH_VARS
      CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR MLN_FFI_CMAKE_RUNTIME_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)
  install(
    FILES "${config}"
    DESTINATION "${config_dir}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  set(${out_template} "${config}.in" PARENT_SCOPE)
  set(${out_config} "${config}" PARENT_SCOPE)
endfunction()
