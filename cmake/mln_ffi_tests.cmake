# Builds the native suites under tests/native and registers how each target
# runs them. See tests/native/README.md for the layout and the rules.

set(MLN_NATIVE_TESTS_DIR "${PROJECT_SOURCE_DIR}/tests/native")

# The ABI suite's domains, one directory each under tests/native/abi, in the
# order the registry runs them. Plugin runs last: a plugin registration lasts
# for the rest of the process.
set(MLN_NATIVE_ABI_DOMAINS
    base
    completion
    runtime
    resources
    map
    projection
    style
    render
    backend
    adapter
    platform
    plugin)

# The browser runs the ABI suite as four pages rather than one per file, which
# would launch Chromium once per file. Each shard names its domains.
set(MLN_NATIVE_BROWSER_SHARDS "base,completion,runtime"
    "resources,map,projection,style" "render,backend" "adapter,platform,plugin")

# Tags a test file can end its name with to build only on matching targets,
# such as render/retarget_metal.c or platform/browser_http_emscripten.c.
set(MLN_NATIVE_FILE_TAGS metal opengl vulkan webgpu egl wgl webgl emscripten)

function(mln_native_active_file_tags out_var)
  set(tags ${MLN_FFI_RENDER_BACKEND})
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    list(APPEND tags ${MLN_FFI_OPENGL_CONTEXT_PROVIDER})
  endif()
  if(EMSCRIPTEN)
    list(APPEND tags emscripten)
  endif()
  set(${out_var} ${tags} PARENT_SCOPE)
endfunction()

# Collects the ABI suite's test files for this target, in registry order, and
# the group symbol each one defines.
function(mln_native_collect_abi_tests out_sources out_symbols)
  mln_native_active_file_tags(active_tags)
  file(GLOB_RECURSE all_sources RELATIVE "${MLN_NATIVE_TESTS_DIR}/abi"
       CONFIGURE_DEPENDS "${MLN_NATIVE_TESTS_DIR}/abi/*.c")
  foreach(relative IN LISTS all_sources)
    string(REGEX MATCH "^([^/]+)/[^/]+$" matched "${relative}")
    if(NOT matched
       OR NOT CMAKE_MATCH_1 IN_LIST MLN_NATIVE_ABI_DOMAINS)
      message(
        FATAL_ERROR
          "tests/native/abi/${relative} is outside the domain directories; "
          "move it under one of: ${MLN_NATIVE_ABI_DOMAINS}")
    endif()
  endforeach()

  set(sources)
  set(symbols)
  foreach(domain IN LISTS MLN_NATIVE_ABI_DOMAINS)
    set(domain_sources ${all_sources})
    list(FILTER domain_sources INCLUDE REGEX "^${domain}/")
    list(SORT domain_sources)
    foreach(relative IN LISTS domain_sources)
      get_filename_component(stem "${relative}" NAME_WE)
      string(REGEX MATCH "_([a-z0-9]+)$" tagged "${stem}")
      if(
        tagged
        AND
        CMAKE_MATCH_1
        IN_LIST
        MLN_NATIVE_FILE_TAGS
        AND
        NOT
        CMAKE_MATCH_1
        IN_LIST
        active_tags)
        continue()
      endif()
      string(MAKE_C_IDENTIFIER "${domain}_${stem}" identifier)
      list(APPEND sources "${relative}")
      list(APPEND symbols "mln_test_group_${identifier}")
    endforeach()
  endforeach()
  set(${out_sources} ${sources} PARENT_SCOPE)
  set(${out_symbols} ${symbols} PARENT_SCOPE)
endfunction()

function(mln_native_fetch_unity)
  include(FetchContent)
  fetchcontent_declare(
    unity
    URL https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.6.1.tar.gz
    URL_HASH
      SHA256=b41a66d45a6b99758fb3202ace6178177014d52fc524bf1f72687d93e9867292
    EXCLUDE_FROM_ALL)
  fetchcontent_makeavailable(unity)
  # Unity's own translation unit and every test must agree on its options, so
  # both read tests/native/support/unity_config.h.
  target_compile_definitions(unity PUBLIC UNITY_INCLUDE_CONFIG_H)
  # Unity's project installs an export set, so the path stays build-only.
  target_include_directories(
    unity
    PUBLIC "$<BUILD_INTERFACE:${MLN_NATIVE_TESTS_DIR}/support>")
  if(MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Unity's Unix -Wall flag means -Weverything to clang-cl. Neutralize it to
    # match the framework's MSVC warning behavior.
    target_compile_options(unity PRIVATE -Wno-everything)
  endif()
endfunction()

# The plugin the plugin group registers. It is its own shared library that
# includes only upstream's plugin header and registers through the function
# pointer the host passes in, which is how a plugin reaches this library in a
# real deployment.
function(mln_native_add_test_plugin)
  get_target_property(shared_supported mln_ffi_platform_dependencies
                      MLN_FFI_SHARED_SUPPORTED)
  if(shared_supported)
    add_library(mln_native_test_plugin SHARED)
  else()
    add_library(mln_native_test_plugin STATIC)
  endif()
  target_sources(
    mln_native_test_plugin
    PRIVATE "${MLN_NATIVE_TESTS_DIR}/plugin/square_plugin.c")
  set_target_properties(
    mln_native_test_plugin
    PROPERTIES
      OUTPUT_NAME
      mln-native-test-plugin
      C_STANDARD
      23
      C_STANDARD_REQUIRED
      YES
      C_EXTENSIONS
      OFF
      C_VISIBILITY_PRESET
      hidden)
  target_include_directories(
    mln_native_test_plugin
    PUBLIC "${MLN_NATIVE_TESTS_DIR}/plugin"
    PRIVATE "${MLN_FFI_SOURCE_DIR}/include")
  target_compile_definitions(
    mln_native_test_plugin
    PRIVATE MLN_NATIVE_TEST_PLUGIN_BUILDING)
  if(NOT shared_supported)
    target_compile_definitions(
      mln_native_test_plugin
      PUBLIC MLN_NATIVE_TEST_PLUGIN_STATIC)
  endif()
endfunction()

function(mln_native_configure_browser_abi_test)
  # The suite runs as a page, which needs three things a native run gets for
  # free:
  #
  #   * A canvas, because the OpenGL fixture creates a real WebGL2 context.
  #   * Its fixtures, which it opens through stdio. They are embedded in the
  #     module rather than served, so the suite reads them the same way it
  #     does everywhere else.
  #   * Cross-origin isolation. The build uses pthreads, so SharedArrayBuffer
  #     has to be available, which means COOP/COEP response headers and
  #     therefore a real HTTP origin rather than file://. The runner serves the
  #     directory.
  set_target_properties(mln_native_abi_tests PROPERTIES SUFFIX ".html")
  target_link_options(
    mln_native_abi_tests
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
      # ENV for the fixture origin and timeout scale the page shell hands the
      # suite.
      "-sEXPORTED_RUNTIME_METHODS=GL,ENV"
      "SHELL:--shell-file ${MLN_NATIVE_TESTS_DIR}/support/browser_shell.html"
      # Unity reports through stdout and the process exit status, so the runner
      # needs the module to exit rather than keep its runtime alive.
      "-sEXIT_RUNTIME=1"
      "SHELL:--embed-file ${MLN_NATIVE_TESTS_DIR}/fixtures@/fixtures")

  find_program(MLN_FFI_NODE_EXECUTABLE node REQUIRED)
  set(shard_index 0)
  foreach(shard IN LISTS MLN_NATIVE_BROWSER_SHARDS)
    math(EXPR shard_index "${shard_index} + 1")
    string(REPLACE "," ";" shard_domains "${shard}")
    set(filter)
    foreach(domain IN LISTS shard_domains)
      list(APPEND filter "/abi/${domain}/")
    endforeach()
    list(JOIN filter "," filter)
    set(test_name "native-abi/browser-shard-${shard_index}")
    # The runner's timeout sits below the entry's, so a run that hangs ends at
    # the runner, which reports how far the shard got and removes the browser
    # profile, rather than at CTest.
    add_test(
      NAME ${test_name}
      COMMAND
        "${MLN_FFI_NODE_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/scripts/run-browser-test.mjs"
        "$<TARGET_FILE:mln_native_abi_tests>"
        --timeout-seconds
        170
        --render-backend
        ${MLN_FFI_RENDER_BACKEND}
        --module-arg
        -f
        --module-arg
        "${filter}")
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 180)
  endforeach()
endfunction()

# Environment every native test process gets from CTest.
function(mln_native_test_environment out_environment out_modifications)
  get_target_property(dependency_runtime_dirs mln_ffi_render_dependencies
                      MLN_FFI_RUNTIME_DIRS)
  if("${dependency_runtime_dirs}" MATCHES "-NOTFOUND$")
    set(dependency_runtime_dirs "")
  endif()
  set(modifications)
  get_target_property(test_library_path_variable mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LIBRARY_PATH_VARIABLE)
  get_target_property(platform_runtime_dirs mln_ffi_platform_dependencies
                      MLN_FFI_TEST_RUNTIME_DIRS)
  if(test_library_path_variable)
    foreach(runtime_dir IN LISTS platform_runtime_dirs dependency_runtime_dirs)
      list(APPEND modifications
           "${test_library_path_variable}=path_list_prepend:${runtime_dir}")
    endforeach()
  endif()
  # The directory arrives at run time, which keeps the checkout path out of the
  # compiled objects: an object carrying it would send a second checkout reading
  # fixtures out of the tree that happened to compile it first.
  set(environment "MLN_FFI_TEST_FIXTURE_DIR=${MLN_NATIVE_TESTS_DIR}/fixtures")
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  if(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    list(APPEND environment "MLN_FFI_SIMULATOR_RUNTIME=tvOS")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT maccatalyst)
    list(APPEND environment "MLN_FFI_SIMULATOR_RUNTIME=iOS")
  endif()
  get_target_property(vulkan_icd_file mln_ffi_render_dependencies
                      MLN_FFI_VULKAN_ICD_FILE)
  if(vulkan_icd_file)
    list(APPEND environment "VK_ICD_FILENAMES=${vulkan_icd_file}")
  endif()
  set(${out_environment} ${environment} PARENT_SCOPE)
  set(${out_modifications} ${modifications} PARENT_SCOPE)
endfunction()

# The fault-injection harness links statically so operator new replacements
# apply to native disposal admission without altering the shipped library.
function(mln_native_add_internal_tests)
  if(NOT TARGET maplibre_native_c_static OR CMAKE_CROSSCOMPILING)
    return()
  endif()
  add_executable(mln_ffi_disposal_allocation_test
                 "${MLN_NATIVE_TESTS_DIR}/internal/disposal_allocation.cpp")
  target_compile_features(mln_ffi_disposal_allocation_test PRIVATE cxx_std_20)
  target_compile_definitions(
    mln_ffi_disposal_allocation_test
    PRIVATE $<TARGET_PROPERTY:maplibre_native_c_objects,COMPILE_DEFINITIONS>)
  target_include_directories(
    mln_ffi_disposal_allocation_test
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
      $<TARGET_PROPERTY:maplibre_native_c_objects,INCLUDE_DIRECTORIES>)
  target_include_directories(
    mln_ffi_disposal_allocation_test
    SYSTEM
    PRIVATE
      $<TARGET_PROPERTY:maplibre_native_c_objects,SYSTEM_INCLUDE_DIRECTORIES>)
  target_link_libraries(
    mln_ffi_disposal_allocation_test
    PRIVATE maplibre_native_c_static)
  add_test(
    NAME native-internal/disposal-allocation
    COMMAND mln_ffi_disposal_allocation_test)
  set_tests_properties(
    native-internal/disposal-allocation
    PROPERTIES TIMEOUT 120)
endfunction()

function(mln_ffi_add_native_tests)
  find_program(MLN_FFI_UV_EXECUTABLE uv REQUIRED)
  add_test(
    NAME binding-contracts
    COMMAND
      ${MLN_FFI_UV_EXECUTABLE}
      run
      --no-sync
      python
      -m
      tools.bindgen
      validate)
  set_tests_properties(
    binding-contracts
    PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  get_target_property(test_supported mln_ffi_platform_dependencies
                      MLN_FFI_TEST_SUPPORTED)
  if(NOT test_supported)
    return()
  endif()
  get_target_property(dependency_include_dirs mln_ffi_render_dependencies
                      MLN_FFI_INCLUDE_DIRS)
  # Libraries the harness links for the graphics API it drives itself, which the
  # C API resolves at runtime rather than linking.
  get_target_property(dependency_test_libraries mln_ffi_render_dependencies
                      MLN_FFI_TEST_LINK_LIBRARIES)
  if("${dependency_include_dirs}" MATCHES "-NOTFOUND$")
    set(dependency_include_dirs "")
  endif()
  if("${dependency_test_libraries}" MATCHES "-NOTFOUND$")
    set(dependency_test_libraries "")
  endif()

  mln_native_fetch_unity()
  mln_native_add_internal_tests()
  mln_native_add_test_plugin()

  mln_native_collect_abi_tests(abi_sources abi_symbols)
  set(registry "")
  set(test_sources)
  foreach(relative symbol IN ZIP_LISTS abi_sources abi_symbols)
    set(source "${MLN_NATIVE_TESTS_DIR}/abi/${relative}")
    list(APPEND test_sources "${source}")
    set_property(
      SOURCE "${source}"
      APPEND
      PROPERTY COMPILE_DEFINITIONS "MLN_TEST_GROUP_NAME=${symbol}")
    string(APPEND registry
           "MLN_TEST_REGISTRY_ENTRY(${symbol}, \"tests/native/abi/${relative}\")\n")
  endforeach()
  set(registry_dir "${CMAKE_CURRENT_BINARY_DIR}/tests/native")
  file(
    CONFIGURE
    OUTPUT "${registry_dir}/mln_native_test_registry.inc"
    CONTENT "${registry}")

  set(support_dir "${MLN_NATIVE_TESTS_DIR}/support")
  # The render fixture's context comes from the file for this preset's backend.
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    set(render_backend_file "render_${MLN_FFI_OPENGL_CONTEXT_PROVIDER}.c")
  else()
    set(render_backend_file "render_${MLN_FFI_RENDER_BACKEND}.c")
  endif()
  set(support_sources
      "${support_dir}/harness.c"
      "${support_dir}/watchdog.cpp"
      "${support_dir}/wait.cpp"
      "${support_dir}/env.c"
      "${support_dir}/render.c"
      "${support_dir}/${render_backend_file}"
      "${support_dir}/hooks.cpp")
  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    list(APPEND support_sources "${support_dir}/metal_retarget.mm")
    set_source_files_properties(
      "${support_dir}/metal_retarget.mm"
      PROPERTIES COMPILE_OPTIONS -fobjc-arc)
  endif()

  add_executable(mln_native_abi_tests ${support_sources} ${test_sources})
  set_target_properties(
    mln_native_abi_tests
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
  # The suite links the shipped library so it exercises the same export
  # boundary hosts link against. The one symbol it needs beyond the public API
  # is the blocking driver hook, which the library exports under
  # MLN_FFI_ENABLE_TEST_HOOKS and only in a test build.
  target_compile_definitions(
    maplibre_native_c_objects
    PRIVATE MLN_FFI_ENABLE_TEST_HOOKS)
  target_link_libraries(
    mln_native_abi_tests
    PRIVATE
      maplibre_native_c unity::framework MLN_FFI::RenderDependencies
      mln_native_test_plugin ${dependency_test_libraries})
  target_include_directories(
    mln_native_abi_tests
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src "${MLN_NATIVE_TESTS_DIR}" "${support_dir}"
      "${registry_dir}" ${dependency_include_dirs})
  target_include_directories(
    mln_native_abi_tests
    SYSTEM
    PRIVATE
      ${MLN_FFI_SOURCE_DIR}/include
      ${MLN_FFI_SOURCE_DIR}/vendor/maplibre-native-base/include)

  # A case that no RUN_TEST references is an unused static function, and
  # dropping `static` to dodge that trips the missing-prototype error instead,
  # because the support headers declare every function the suite exports.
  # These stay off the vendored unity target.
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
      mln_native_abi_tests
      PRIVATE -Werror=unused-function -Werror=missing-prototypes)
  elseif(MSVC)
    # C4505: unreferenced function with internal linkage has been removed.
    target_compile_options(mln_native_abi_tests PRIVATE /we4505)
  endif()

  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    target_compile_definitions(
      mln_native_abi_tests
      PRIVATE MLN_FFI_TEST_BACKEND_METAL=1)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    target_compile_definitions(
      mln_native_abi_tests
      PRIVATE MLN_FFI_TEST_BACKEND_OPENGL=1)
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
      target_compile_definitions(
        mln_native_abi_tests
        PRIVATE MLN_FFI_TEST_OPENGL_WGL=1)
    elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
      target_compile_definitions(
        mln_native_abi_tests
        PRIVATE MLN_FFI_TEST_OPENGL_WEBGL=1)
    else()
      target_compile_definitions(
        mln_native_abi_tests
        PRIVATE MLN_FFI_TEST_OPENGL_EGL=1)
    endif()
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    target_compile_definitions(
      mln_native_abi_tests
      PRIVATE MLN_FFI_TEST_BACKEND_VULKAN=1)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
    target_compile_definitions(
      mln_native_abi_tests
      PRIVATE MLN_FFI_TEST_BACKEND_WEBGPU=1)
  endif()

  if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(mln_native_abi_tests PRIVATE Threads::Threads)
  endif()
  # The Metal test support defines an Objective-C class, which the iOS and
  # tvOS test app links against the runtime and Foundation directly.
  if(APPLE AND MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    target_link_libraries(
      mln_native_abi_tests
      PRIVATE "-framework Foundation" objc)
  endif()

  get_target_property(test_link_options mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LINK_OPTIONS)
  if(test_link_options)
    target_link_options(mln_native_abi_tests PRIVATE ${test_link_options})
  endif()

  if(EMSCRIPTEN)
    mln_native_configure_browser_abi_test()
    return()
  endif()

  # Each registered test gets the same environment, so a new entry cannot miss
  # the fixture directory or the loader path.
  mln_native_test_environment(test_environment test_modifications)
  set(registered_tests)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS)$" AND NOT maccatalyst)
    # A simulator spawns one process per run, so the suite runs as one entry,
    # with the plugin group in an invocation of its own after it.
    set(runner bash "${PROJECT_SOURCE_DIR}/scripts/run-ios-simulator-test.sh"
        "$<TARGET_FILE:mln_native_abi_tests>" 300)
    add_test(NAME native-abi COMMAND ${runner} -- -x /abi/plugin/)
    add_test(NAME native-abi/plugin COMMAND ${runner} -- -f /abi/plugin/)
    set_tests_properties(native-abi PROPERTIES TIMEOUT 300)
    set_tests_properties(
      native-abi/plugin
      PROPERTIES DEPENDS native-abi TIMEOUT 120)
    list(APPEND registered_tests native-abi native-abi/plugin)
  else()
    # One entry per file, so `ctest --parallel` spreads the suite across
    # processes and a crash takes down one file rather than the run.
    foreach(relative IN LISTS abi_sources)
      string(REGEX REPLACE "\\.c$" "" test_path "${relative}")
      set(test_name "native-abi/${test_path}")
      add_test(
        NAME ${test_name}
        COMMAND mln_native_abi_tests -f "/abi/${relative}")
      set_tests_properties(${test_name} PROPERTIES TIMEOUT 120)
      list(APPEND registered_tests ${test_name})
    endforeach()
  endif()
  if(TARGET mln_ffi_disposal_allocation_test)
    list(APPEND registered_tests native-internal/disposal-allocation)
  endif()
  foreach(test_name IN LISTS registered_tests)
    set_property(TEST ${test_name} PROPERTY ENVIRONMENT ${test_environment})
    if(test_modifications)
      set_property(
        TEST ${test_name}
        PROPERTY ENVIRONMENT_MODIFICATION ${test_modifications})
    endif()
  endforeach()
endfunction()
