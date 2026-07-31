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
  # Libraries the harness links for the graphics API it drives itself, which the
  # C API resolves at runtime rather than linking.
  get_target_property(dependency_test_libraries mln_ffi_render_dependencies
                      MLN_FFI_TEST_LINK_LIBRARIES)
  if("${dependency_runtime_dirs}" MATCHES "-NOTFOUND$")
    set(dependency_runtime_dirs "")
  endif()
  if("${dependency_include_dirs}" MATCHES "-NOTFOUND$")
    set(dependency_include_dirs "")
  endif()
  if("${dependency_test_libraries}" MATCHES "-NOTFOUND$")
    set(dependency_test_libraries "")
  endif()

  include(FetchContent)
  fetchcontent_declare(
    unity
    URL https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.6.1.tar.gz
    URL_HASH
      SHA256=b41a66d45a6b99758fb3202ace6178177014d52fc524bf1f72687d93e9867292
    EXCLUDE_FROM_ALL)
  fetchcontent_makeavailable(unity)
  # Unity buffers its per-test lines when stdout is a pipe, so a run that never
  # returns takes its progress with it and ctest reports a timeout with no
  # output. Flushing each line leaves the last test it started in the log. The
  # flush calls live in Unity's own translation unit, so the definition belongs
  # on that target rather than on the tests that link it.
  target_compile_definitions(unity PRIVATE UNITY_USE_FLUSH_STDOUT=1)
  # Unity 2.6 makes double support opt-in: without this define TEST_ASSERT_*
  # DOUBLE macros expand to an unconditional "Double Support Disabled" failure.
  # Unity's own translation unit and every test that includes unity.h have to
  # agree on it, so unlike UNITY_USE_FLUSH_STDOUT (which only affects calls
  # inside Unity's sources) this one is PUBLIC.
  target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
  if(MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Unity's Unix -Wall flag means -Weverything to clang-cl. Neutralize it to
    # match the framework's MSVC warning behavior.
    target_compile_options(unity PRIVATE -Wno-everything)
  endif()

  # Globbing keeps a newly added *_abi.c in the build without a second edit
  # here; CONFIGURE_DEPENDS reruns the glob when the directory changes.
  file(GLOB test_abi_sources CONFIGURE_DEPENDS
       ${PROJECT_SOURCE_DIR}/src/c_api/tests/*_abi.c)
  set(test_sources ${PROJECT_SOURCE_DIR}/src/c_api/tests/main.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/test_support.c ${test_abi_sources})

  # Each *_abi.c reaches the runner through one run_<file>_tests() call in
  # main.c. main.c carries no preprocessor guards, so a plain text match is
  # exact here once comments are stripped -- otherwise a commented-out call
  # would satisfy the guard while its tests never run. See
  # src/c_api/tests/README.md for the full contract.
  file(READ ${PROJECT_SOURCE_DIR}/src/c_api/tests/main.c test_main_contents)
  string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*+/" "" test_main_contents
         "${test_main_contents}")
  string(REGEX REPLACE "//[^\n]*" "" test_main_contents "${test_main_contents}")
  # String literals too, so a runner named only inside a diagnostic message
  # cannot stand in for the call that runs it.
  string(REGEX REPLACE "\"([^\"\\\\]|\\\\.)*\"" "" test_main_contents
         "${test_main_contents}")
  foreach(test_abi_source IN LISTS test_abi_sources)
    get_filename_component(test_abi_name ${test_abi_source} NAME_WE)
    if(NOT test_main_contents MATCHES "run_${test_abi_name}_tests\\(\\)")
      message(
        FATAL_ERROR
          "src/c_api/tests/${test_abi_name}.c defines tests that never run: "
          "declare run_${test_abi_name}_tests(void) in "
          "src/c_api/tests/abi_tests.h and call it from "
          "src/c_api/tests/main.c.")
    endif()
  endforeach()

  add_executable(mln_c_api_tests ${test_sources})
  set_target_properties(
    mln_c_api_tests
    PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
  target_link_libraries(
    mln_c_api_tests
    PRIVATE
      maplibre_native_c unity::framework MLN_FFI::RenderDependencies
      ${dependency_test_libraries})
  target_include_directories(
    mln_c_api_tests
    PRIVATE ${PROJECT_SOURCE_DIR}/src/c_api/tests ${dependency_include_dirs})

  # Enforce the registration contract at compile time. A test that no RUN_TEST
  # references is an unused static function, and dropping `static` to dodge that
  # trips the missing-prototype error instead, because abi_tests.h and
  # test_support.h declare every function this suite legitimately exports.
  # These stay off the vendored unity target.
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
      mln_c_api_tests
      PRIVATE -Werror=unused-function -Werror=missing-prototypes)
  elseif(MSVC)
    # C4505: unreferenced function with internal linkage has been removed.
    target_compile_options(mln_c_api_tests PRIVATE /we4505)
  endif()

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

  get_target_property(test_link_options mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LINK_OPTIONS)
  if(test_link_options)
    target_link_options(mln_c_api_tests PRIVATE ${test_link_options})
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
  # Tile fixtures the suite feeds through a resource provider come from the
  # MapLibre Native submodule rather than being duplicated here, so a submodule
  # bump that moves one surfaces as a test failure instead of silent drift. The
  # directory arrives at run time, which keeps the checkout path out of the
  # compiled objects: an object carrying it would send a second checkout reading
  # fixtures out of the tree that happened to compile it first.
  set(test_environment
      "MLN_TEST_FIXTURE_DIR=${PROJECT_SOURCE_DIR}/third_party/maplibre-native/test/fixtures")
  get_target_property(vulkan_icd_file mln_ffi_render_dependencies
                      MLN_FFI_VULKAN_ICD_FILE)
  if(vulkan_icd_file)
    list(APPEND test_environment "VK_ICD_FILENAMES=${vulkan_icd_file}")
  endif()
  set_property(TEST c-api PROPERTY ENVIRONMENT ${test_environment})
endfunction()
