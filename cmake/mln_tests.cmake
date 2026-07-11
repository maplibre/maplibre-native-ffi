function(mln_add_c_api_test)
  get_target_property(test_supported mln_ffi_platform_dependencies
                      MLN_FFI_TEST_SUPPORTED)
  if(NOT test_supported)
    return()
  endif()
  get_target_property(zig_target mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_TARGET)

  get_target_property(dependency_include_dirs mln_ffi_render_dependencies
                      MLN_FFI_INCLUDE_DIRS)
  get_target_property(dependency_library_dirs mln_ffi_render_dependencies
                      MLN_FFI_LIBRARY_DIRS)
  get_target_property(dependency_runtime_dirs mln_ffi_render_dependencies
                      MLN_FFI_RUNTIME_DIRS)

  foreach(variable IN ITEMS dependency_include_dirs dependency_library_dirs
          dependency_runtime_dirs)
    if("${${variable}}" MATCHES "-NOTFOUND$")
      set(${variable} "")
    endif()
  endforeach()

  set(zig_args
      build test "-Dtarget=${zig_target}"
      "-Dnative-install-dir=${CMAKE_INSTALL_PREFIX}"
      "-Drender-backend=${MLN_FFI_RENDER_BACKEND}")
  foreach(dependency_include_dir IN LISTS dependency_include_dirs)
    list(APPEND zig_args "-Ddependency-include-dir=${dependency_include_dir}")
  endforeach()
  foreach(dependency_library_dir IN LISTS dependency_library_dirs)
    list(APPEND zig_args "-Ddependency-library-dir=${dependency_library_dir}")
  endforeach()

  get_target_property(test_system_root mln_ffi_platform_dependencies
                      MLN_FFI_TEST_SYSTEM_ROOT)
  if(test_system_root)
    list(APPEND zig_args "-Dsystem-root=${test_system_root}")
  endif()
  get_target_property(zig_libc_sysroot mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_LIBC_SYSROOT)
  if(zig_libc_sysroot)
    list(APPEND zig_args --libc
         "${CMAKE_INSTALL_PREFIX}/share/maplibre-native-c/zig-libc")
  endif()
  list(
    APPEND
    zig_args
    --test-timeout
    120s
    --summary
    all
    --verbose)

  add_test(
    NAME c-api
    COMMAND zig ${zig_args}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/c_api/tests/zig")
  set_tests_properties(c-api PROPERTIES TIMEOUT 180)

  get_target_property(test_library_path_variable mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LIBRARY_PATH_VARIABLE)
  get_target_property(platform_runtime_dirs mln_ffi_platform_dependencies
                      MLN_FFI_TEST_RUNTIME_DIRS)
  if(test_library_path_variable)
    set(runtime_environment "")
    foreach(runtime_dir IN LISTS platform_runtime_dirs dependency_runtime_dirs)
      list(APPEND runtime_environment
           "${test_library_path_variable}=path_list_prepend:${runtime_dir}")
    endforeach()
    set_property(
      TEST c-api
      PROPERTY ENVIRONMENT_MODIFICATION ${runtime_environment})
  endif()
  get_target_property(vulkan_icd_file mln_ffi_render_dependencies
                      MLN_FFI_VULKAN_ICD_FILE)
  if(vulkan_icd_file)
    set_property(
      TEST c-api
      PROPERTY ENVIRONMENT "VK_ICD_FILENAMES=${vulkan_icd_file}")
  endif()
endfunction()
