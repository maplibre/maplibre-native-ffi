function(mln_add_c_api_test)
  get_target_property(test_supported mln_ffi_platform_dependencies
                      MLN_FFI_TEST_SUPPORTED)
  if(NOT test_supported)
    return()
  endif()
  get_target_property(dependency_runtime_dirs mln_ffi_render_dependencies
                      MLN_FFI_RUNTIME_DIRS)
  get_target_property(dependency_include_dirs mln_ffi_render_dependencies
                      MLN_FFI_INCLUDE_DIRS)
  if("${dependency_runtime_dirs}" MATCHES "-NOTFOUND$")
    set(dependency_runtime_dirs "")
  endif()
  if("${dependency_include_dirs}" MATCHES "-NOTFOUND$")
    set(dependency_include_dirs "")
  endif()

  include(FetchContent)
  fetchcontent_declare(
    unity
    URL https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.6.1.tar.gz
    URL_HASH
      SHA256=b41a66d45a6b99758fb3202ace6178177014d52fc524bf1f72687d93e9867292
    EXCLUDE_FROM_ALL)
  fetchcontent_makeavailable(unity)

  set(test_sources
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/main.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/test_support.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/core_abi.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/map_options_abi.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/render_backend_abi.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/owned_texture_abi.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/query_abi.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/resources_abi.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/style_values_abi.c)
  add_executable(mln_c_api_tests ${test_sources})
  set_target_properties(
    mln_c_api_tests
    PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
  target_link_libraries(
    mln_c_api_tests
    PRIVATE maplibre_native_c unity::framework MLN_FFI::RenderDependencies)
  target_include_directories(
    mln_c_api_tests
    PRIVATE ${PROJECT_SOURCE_DIR}/src/c_api/tests ${dependency_include_dirs})

  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    target_compile_definitions(mln_c_api_tests PRIVATE MLN_TEST_BACKEND_METAL=1)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    target_compile_definitions(
      mln_c_api_tests
      PRIVATE MLN_TEST_BACKEND_OPENGL=1)
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
      target_compile_definitions(mln_c_api_tests PRIVATE MLN_TEST_OPENGL_WGL=1)
    else()
      target_compile_definitions(mln_c_api_tests PRIVATE MLN_TEST_OPENGL_EGL=1)
    endif()
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    target_compile_definitions(
      mln_c_api_tests
      PRIVATE MLN_TEST_BACKEND_VULKAN=1)
  endif()

  if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(mln_c_api_tests PRIVATE Threads::Threads)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_test(
      NAME c-api
      COMMAND
        bash ${PROJECT_SOURCE_DIR}/scripts/run-ios-simulator-test.sh
        $<TARGET_FILE:mln_c_api_tests>)
  else()
    add_test(NAME c-api COMMAND mln_c_api_tests)
  endif()

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
