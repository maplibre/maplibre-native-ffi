# Builds the C API suite as a page and registers the runner that drives it.
#
# The suite needs three things a native run gets for free:
#
#   * A canvas, because the OpenGL fixture creates a real WebGL2 context. emcc's
#     default HTML shell supplies one, so the suite links to .html.
#   * Its tile fixtures, which it opens through stdio. They are embedded in the
#     module rather than served, so the suite reads them the same way it does
#     everywhere else.
#   * Cross-origin isolation. The build uses pthreads, so SharedArrayBuffer has
#     to be available, which means COOP/COEP response headers and therefore a
#     real HTTP origin rather than file://. The runner serves the directory.
function(mln_ffi_configure_browser_c_api_test)
  set(fixture_dir
      "${PROJECT_SOURCE_DIR}/third_party/maplibre-native/test/fixtures")
  # Only the tile fixtures the suite decodes travel into the module. Embedding
  # the whole fixtures tree would carry 81MB of unrelated render-test images.
  file(GLOB_RECURSE embedded_fixtures RELATIVE "${fixture_dir}"
       CONFIGURE_DEPENDS "${fixture_dir}/*.mlt")
  if(NOT embedded_fixtures)
    message(FATAL_ERROR "no MLT tile fixtures found under ${fixture_dir}")
  endif()
  # SHELL: keeps each flag with its value: CMake deduplicates repeated link
  # options, which would otherwise collapse the second --embed-file and leave
  # its path standing alone as an input file.
  set(embed_options)
  foreach(fixture IN LISTS embedded_fixtures)
    list(APPEND embed_options
         "SHELL:--embed-file ${fixture_dir}/${fixture}@/fixtures/${fixture}")
  endforeach()
  set_target_properties(mln_ffi_c_api_tests PROPERTIES SUFFIX ".html")
  target_link_options(
    mln_ffi_c_api_tests
    PRIVATE
      "-sENVIRONMENT=web,worker"
      # main() runs on a worker, where blocking is legal. MapLibre blocks in
      # waitForEmpty() and during teardown, which the browser main thread
      # forbids, so this is what lets the suite run as written.
      "-sPROXY_TO_PTHREAD"
      # Fixtures create a private OffscreenCanvas per session, on whichever
      # worker attaches it, and register it in GL.offscreenCanvases -- which is
      # what resolves the selector, so that table has to be reachable from JS.
      "-sOFFSCREENCANVAS_SUPPORT=1"
      # GL for the canvas registry the fixtures resolve their selector through,
      # ENV for the fixture origin the page shell hands the HTTP test.
      "-sEXPORTED_RUNTIME_METHODS=GL,ENV"
      "SHELL:--shell-file ${PROJECT_SOURCE_DIR}/src/c_api/tests/browser_shell.html"
      # Unity reports through stdout and the process exit status, so the runner
      # needs the module to exit rather than keep its runtime alive.
      "-sEXIT_RUNTIME=1"
      ${embed_options})
  # Below the CTest timeout the browser test presets inherit, so a run that
  # hangs ends at the runner rather than at CTest. The runner reports how far
  # the suite got, kills the browser, and removes its profile, so letting CTest
  # kill it first costs the diagnosis and leaves the profile behind.
  set(runner_timeout_seconds 240)

  # Keep backend-specific browser flags in the shared runner.
  set(browser_args --render-backend ${MLN_FFI_RENDER_BACKEND})

  find_program(MLN_FFI_NODE_EXECUTABLE node REQUIRED)
  add_test(
    NAME c-api
    COMMAND
      "${MLN_FFI_NODE_EXECUTABLE}"
      "${PROJECT_SOURCE_DIR}/scripts/run-browser-test.mjs"
      "$<TARGET_FILE:mln_ffi_c_api_tests>" --timeout-seconds
      ${runner_timeout_seconds} ${browser_args})
endfunction()

function(mln_ffi_add_c_api_test)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(
    NAME execution-conventions
    COMMAND
      ${Python3_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/scripts/check-execution-conventions.py
      ${PROJECT_SOURCE_DIR}/include/maplibre_native_c)
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
  # Unity infers 64-bit assertion support from pointer width. Handles in this C
  # API are 64 bits wherever they are, so a 32-bit target still needs the UINT64
  # assertions. Like UNITY_INCLUDE_DOUBLE this has to be PUBLIC, because Unity's
  # own translation unit and every test that includes unity.h must agree on it.
  if(CMAKE_SIZEOF_VOID_P LESS 8)
    target_compile_definitions(unity PUBLIC UNITY_SUPPORT_64)
  endif()
  if(MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Unity's Unix -Wall flag means -Weverything to clang-cl. Neutralize it to
    # match the framework's MSVC warning behavior.
    target_compile_options(unity PRIVATE -Wno-everything)
  endif()

  # Globbing keeps a newly added *_abi.c in the build without a second edit
  # here; CONFIGURE_DEPENDS reruns the glob when the directory changes.
  file(GLOB test_abi_sources CONFIGURE_DEPENDS
       ${PROJECT_SOURCE_DIR}/src/c_api/tests/*_abi.c)
  set(test_sources
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/main.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/test_support.c
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/test_support.cpp ${test_abi_sources})
  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    list(APPEND test_sources
         ${PROJECT_SOURCE_DIR}/src/c_api/tests/metal_surface_test_support.mm)
    set_source_files_properties(
      ${PROJECT_SOURCE_DIR}/src/c_api/tests/metal_surface_test_support.mm
      PROPERTIES COMPILE_OPTIONS -fobjc-arc)
  endif()

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

  add_executable(mln_ffi_c_api_tests ${test_sources})
  set_target_properties(
    mln_ffi_c_api_tests
    PROPERTIES
      C_STANDARD
      23
      C_STANDARD_REQUIRED
      YES
      C_EXTENSIONS
      OFF
      CXX_STANDARD
      23
      CXX_STANDARD_REQUIRED
      YES
      CXX_EXTENSIONS
      OFF)
  if(TARGET maplibre_native_c_static)
    set(MLN_FFI_TEST_C_API_TARGET maplibre_native_c_static)
  else()
    set(MLN_FFI_TEST_C_API_TARGET maplibre_native_c)
  endif()
  target_link_libraries(
    mln_ffi_c_api_tests
    PRIVATE
      ${MLN_FFI_TEST_C_API_TARGET} unity::framework MLN_FFI::RenderDependencies
      ${dependency_test_libraries})
  target_include_directories(
    mln_ffi_c_api_tests
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/c_api/tests
      ${dependency_include_dirs})
  target_include_directories(
    mln_ffi_c_api_tests
    SYSTEM
    PRIVATE
      ${MLN_FFI_SOURCE_DIR}/include
      ${MLN_FFI_SOURCE_DIR}/vendor/maplibre-native-base/include)

  # Enforce the registration contract at compile time. A test that no RUN_TEST
  # references is an unused static function, and dropping `static` to dodge that
  # trips the missing-prototype error instead, because abi_tests.h and
  # test_support.h declare every function this suite legitimately exports.
  # These stay off the vendored unity target.
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
      mln_ffi_c_api_tests
      PRIVATE -Werror=unused-function -Werror=missing-prototypes)
  elseif(MSVC)
    # C4505: unreferenced function with internal linkage has been removed.
    target_compile_options(mln_ffi_c_api_tests PRIVATE /we4505)
  endif()

  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    target_compile_definitions(
      mln_ffi_c_api_tests
      PRIVATE MLN_FFI_TEST_BACKEND_METAL=1)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    target_compile_definitions(
      mln_ffi_c_api_tests
      PRIVATE MLN_FFI_TEST_BACKEND_OPENGL=1)
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
      target_compile_definitions(
        mln_ffi_c_api_tests
        PRIVATE MLN_FFI_TEST_OPENGL_WGL=1)
    elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
      target_compile_definitions(
        mln_ffi_c_api_tests
        PRIVATE MLN_FFI_TEST_OPENGL_WEBGL=1)
    else()
      target_compile_definitions(
        mln_ffi_c_api_tests
        PRIVATE MLN_FFI_TEST_OPENGL_EGL=1)
    endif()
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    target_compile_definitions(
      mln_ffi_c_api_tests
      PRIVATE MLN_FFI_TEST_BACKEND_VULKAN=1)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
    target_compile_definitions(
      mln_ffi_c_api_tests
      PRIVATE MLN_FFI_TEST_BACKEND_WEBGPU=1)
  endif()

  if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(mln_ffi_c_api_tests PRIVATE Threads::Threads)
  endif()

  get_target_property(test_link_options mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LINK_OPTIONS)
  if(test_link_options)
    target_link_options(mln_ffi_c_api_tests PRIVATE ${test_link_options})
  endif()

  if(EMSCRIPTEN)
    mln_ffi_configure_browser_c_api_test()
    return()
  endif()

  if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS)$")
    add_test(
      NAME c-api
      COMMAND
        bash ${PROJECT_SOURCE_DIR}/scripts/run-ios-simulator-test.sh
        $<TARGET_FILE:mln_ffi_c_api_tests>)
  else()
    add_test(NAME c-api COMMAND mln_ffi_c_api_tests)
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
      "MLN_FFI_TEST_FIXTURE_DIR=${PROJECT_SOURCE_DIR}/third_party/maplibre-native/test/fixtures")
  if(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    list(APPEND test_environment "MLN_FFI_SIMULATOR_RUNTIME=tvOS")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND test_environment "MLN_FFI_SIMULATOR_RUNTIME=iOS")
  endif()
  get_target_property(vulkan_icd_file mln_ffi_render_dependencies
                      MLN_FFI_VULKAN_ICD_FILE)
  if(vulkan_icd_file)
    list(APPEND test_environment "VK_ICD_FILENAMES=${vulkan_icd_file}")
  endif()
  set_property(TEST c-api PROPERTY ENVIRONMENT ${test_environment})
endfunction()
