# Builds the native suites under tests/native and registers how each target
# runs them. See tests/native/README.md for the layout and the rules.

include(mln_ffi_test_graphics)

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
# such as backend/dedicated_egl.c or platform/browser_http_emscripten.c.
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

# Whether a test file builds on this target, judged by the tag its name ends
# with.
function(mln_native_file_selected relative active_tags out_var)
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
    set(${out_var} FALSE PARENT_SCOPE)
  else()
    set(${out_var} TRUE PARENT_SCOPE)
  endif()
endfunction()

# Collects the ABI suite's test files for this target, in registry order, and
# the group symbol each one defines.
function(mln_native_collect_abi_tests out_sources out_symbols)
  mln_native_active_file_tags(active_tags)
  file(GLOB_RECURSE all_sources RELATIVE "${MLN_NATIVE_TESTS_DIR}/abi"
       CONFIGURE_DEPENDS "${MLN_NATIVE_TESTS_DIR}/abi/*.c")
  foreach(relative IN LISTS all_sources)
    string(REGEX MATCH "^([^/]+)/[^/]+$" matched "${relative}")
    if(NOT matched OR NOT CMAKE_MATCH_1 IN_LIST MLN_NATIVE_ABI_DOMAINS)
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
      mln_native_file_selected("${relative}" "${active_tags}" selected)
      if(NOT selected)
        continue()
      endif()
      get_filename_component(stem "${relative}" NAME_WE)
      string(MAKE_C_IDENTIFIER "${domain}_${stem}" identifier)
      list(APPEND sources "abi/${relative}")
      list(APPEND symbols "mln_test_group_${identifier}")
    endforeach()
  endforeach()
  set(${out_sources} ${sources} PARENT_SCOPE)
  set(${out_symbols} ${symbols} PARENT_SCOPE)
endfunction()

# Collects the internal suite's test files for this target, one group per
# file directly under tests/native/internal. Its helpers live in
# internal/support, which holds no group.
function(mln_native_collect_internal_tests out_sources out_symbols)
  mln_native_active_file_tags(active_tags)
  file(
    GLOB
    all_sources
    RELATIVE
    "${MLN_NATIVE_TESTS_DIR}/internal"
    CONFIGURE_DEPENDS
    "${MLN_NATIVE_TESTS_DIR}/internal/*.cpp"
    "${MLN_NATIVE_TESTS_DIR}/internal/*.mm")
  list(SORT all_sources)
  set(sources)
  set(symbols)
  foreach(relative IN LISTS all_sources)
    mln_native_file_selected("${relative}" "${active_tags}" selected)
    if(NOT selected)
      continue()
    endif()
    get_filename_component(stem "${relative}" NAME_WE)
    string(MAKE_C_IDENTIFIER "internal_${stem}" identifier)
    list(APPEND sources "internal/${relative}")
    list(APPEND symbols "mln_test_group_${identifier}")
    if(relative MATCHES "\\.mm$")
      set_source_files_properties(
        "${MLN_NATIVE_TESTS_DIR}/internal/${relative}"
        PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    endif()
  endforeach()
  set(${out_sources} ${sources} PARENT_SCOPE)
  set(${out_symbols} ${symbols} PARENT_SCOPE)
endfunction()

# Writes the registry the harness walks for one suite, and names each file's
# group through a compile definition. `sources` are relative to
# tests/native.
function(mln_native_write_registry registry_dir sources symbols)
  set(registry "")
  foreach(relative symbol IN ZIP_LISTS sources symbols)
    set_property(
      SOURCE "${MLN_NATIVE_TESTS_DIR}/${relative}"
      APPEND
      PROPERTY COMPILE_DEFINITIONS "MLN_TEST_GROUP_NAME=${symbol}")
    string(APPEND registry
           "MLN_TEST_REGISTRY_ENTRY(${symbol}, \"tests/native/${relative}\")\n")
  endforeach()
  file(
    CONFIGURE
    OUTPUT "${registry_dir}/mln_native_test_registry.inc"
    CONTENT "${registry}")
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

# Links a suite into a page the browser runner loads. It needs three things a
# native run gets for free:
#
#   * A canvas, because the OpenGL fixture creates a real WebGL2 context.
#   * Its fixtures, which it opens through stdio. They are embedded in the
#     module rather than served, so the suite reads them the same way it does
#     everywhere else.
#   * Cross-origin isolation. The build uses pthreads, so SharedArrayBuffer has
#     to be available, which means COOP/COEP response headers and therefore a
#     real HTTP origin rather than file://. The runner serves the directory.
function(mln_native_configure_browser_page target)
  set_target_properties(${target} PROPERTIES SUFFIX ".html")
  target_link_options(
    ${target}
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
endfunction()

# Registers one browser page run. The runner's timeout sits below the entry's,
# so a run that hangs ends at the runner, which reports how far it got and
# removes the browser profile, rather than at CTest.
function(mln_native_add_browser_test test_name target)
  find_program(MLN_FFI_NODE_EXECUTABLE node REQUIRED)
  set(module_arguments)
  foreach(argument IN LISTS ARGN)
    list(APPEND module_arguments --module-arg "${argument}")
  endforeach()
  add_test(
    NAME ${test_name}
    COMMAND
      "${MLN_FFI_NODE_EXECUTABLE}"
      "${PROJECT_SOURCE_DIR}/scripts/run-browser-test.mjs"
      "$<TARGET_FILE:${target}>"
      --timeout-seconds
      170
      --render-backend
      ${MLN_FFI_RENDER_BACKEND}
      ${module_arguments})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 180)
endfunction()

function(mln_native_add_browser_abi_shards)
  set(shard_index 0)
  foreach(shard IN LISTS MLN_NATIVE_BROWSER_SHARDS)
    math(EXPR shard_index "${shard_index} + 1")
    string(REPLACE "," ";" shard_domains "${shard}")
    set(filter)
    foreach(domain IN LISTS shard_domains)
      list(APPEND filter "/abi/${domain}/")
    endforeach()
    list(JOIN filter "," filter)
    mln_native_add_browser_test("native-abi/browser-shard-${shard_index}"
                                mln_native_abi_tests -f "${filter}")
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

# What both suites share: the harness and support sources, Unity, the render
# fixture's graphics dependencies, and the warnings that enforce registration.
function(mln_native_configure_suite target registry_dir)
  get_target_property(dependency_include_dirs mln_ffi_render_dependencies
                      MLN_FFI_INCLUDE_DIRS)
  # Libraries the harness links for the graphics API it drives itself, which
  # the C API resolves at runtime rather than linking.
  get_target_property(dependency_test_libraries mln_ffi_render_dependencies
                      MLN_FFI_TEST_LINK_LIBRARIES)
  if("${dependency_include_dirs}" MATCHES "-NOTFOUND$")
    set(dependency_include_dirs "")
  endif()
  if("${dependency_test_libraries}" MATCHES "-NOTFOUND$")
    set(dependency_test_libraries "")
  endif()

  set(support_dir "${MLN_NATIVE_TESTS_DIR}/support")
  # The render fixture's context comes from tests/graphics on native targets,
  # and from the browser file for this preset's backend in the browser.
  if(EMSCRIPTEN AND MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    set(render_backend_files "render_webgl.c")
  elseif(EMSCRIPTEN)
    set(render_backend_files "render_${MLN_FFI_RENDER_BACKEND}.c")
  else()
    set(render_backend_files "render_graphics.c")
    if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl"
       AND MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
      list(APPEND render_backend_files "render_egl.c")
    endif()
  endif()
  list(TRANSFORM render_backend_files PREPEND "${support_dir}/")
  target_sources(
    ${target}
    PRIVATE
      "${support_dir}/harness.c"
      "${support_dir}/watchdog.cpp"
      "${support_dir}/wait.cpp"
      "${support_dir}/env.c"
      "${support_dir}/render.c"
      "${support_dir}/frames.c"
      "${support_dir}/camera.c"
      "${support_dir}/map.c"
      "${support_dir}/style.c"
      "${support_dir}/resources.c"
      ${render_backend_files})
  # The browser has no sockets, so its transport runs against the runner's
  # routes instead of the loopback server.
  if(NOT EMSCRIPTEN)
    target_sources(${target} PRIVATE "${support_dir}/http_server.c")
  endif()
  if(WIN32)
    target_link_libraries(${target} PRIVATE ws2_32)
  endif()
  if(TARGET mln_test_graphics_objects)
    target_link_libraries(${target} PRIVATE mln_test_graphics_objects)
  endif()
  set_target_properties(
    ${target}
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
  target_link_libraries(
    ${target}
    PRIVATE
      unity::framework MLN_FFI::RenderDependencies ${dependency_test_libraries})
  target_include_directories(
    ${target}
    PRIVATE
      "${MLN_NATIVE_TESTS_DIR}" "${support_dir}" "${registry_dir}"
      ${dependency_include_dirs})
  target_include_directories(
    ${target}
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
      ${target}
      PRIVATE
        -Werror=unused-function
        $<$<COMPILE_LANGUAGE:C,OBJC>:-Werror=missing-prototypes>)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(
        ${target}
        PRIVATE $<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Werror=missing-prototypes>)
    endif()
  elseif(MSVC)
    # C4505: unreferenced function with internal linkage has been removed.
    target_compile_options(${target} PRIVATE /we4505)
  endif()

  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_BACKEND_METAL=1)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_BACKEND_OPENGL=1)
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
      target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_OPENGL_WGL=1)
    elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
      target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_OPENGL_WEBGL=1)
    else()
      target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_OPENGL_EGL=1)
    endif()
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_BACKEND_VULKAN=1)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
    target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_BACKEND_WEBGPU=1)
  endif()

  if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(${target} PRIVATE Threads::Threads)
  endif()
  # The Metal test support defines an Objective-C class, which the iOS and
  # tvOS test app links against the runtime and Foundation directly.
  if(APPLE AND MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    target_link_libraries(${target} PRIVATE "-framework Foundation" objc)
  endif()

  get_target_property(test_link_options mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LINK_OPTIONS)
  if(test_link_options)
    target_link_options(${target} PRIVATE ${test_link_options})
  endif()
  if(EMSCRIPTEN)
    mln_native_configure_browser_page(${target})
  endif()
endfunction()

# The ABI suite includes public headers only and links the shipped library,
# the shared one where the platform builds it, so it exercises the export
# boundary hosts link against.
function(mln_native_add_abi_suite out_sources)
  mln_native_collect_abi_tests(sources symbols)
  set(registry_dir "${CMAKE_CURRENT_BINARY_DIR}/tests/native/abi")
  mln_native_write_registry("${registry_dir}" "${sources}" "${symbols}")
  list(TRANSFORM sources PREPEND "${MLN_NATIVE_TESTS_DIR}/" OUTPUT_VARIABLE
       test_sources)
  add_executable(mln_native_abi_tests ${test_sources})
  mln_native_configure_suite(mln_native_abi_tests "${registry_dir}")
  target_link_libraries(
    mln_native_abi_tests
    PRIVATE maplibre_native_c mln_native_test_plugin)
  set(${out_sources} ${sources} PARENT_SCOPE)
endfunction()

# The internal suite includes src/ headers and links the static library, so a
# case can drive a sync point, fault allocation, or reach an internal module.
# Its objects are the shipped ones: nothing about the library changes for it.
function(mln_native_add_internal_suite out_sources)
  if(TARGET maplibre_native_c_static)
    set(library maplibre_native_c_static)
  else()
    # A platform without shared libraries builds only the static one.
    set(library maplibre_native_c)
  endif()
  mln_native_collect_internal_tests(sources symbols)
  set(registry_dir "${CMAKE_CURRENT_BINARY_DIR}/tests/native/internal")
  mln_native_write_registry("${registry_dir}" "${sources}" "${symbols}")
  list(TRANSFORM sources PREPEND "${MLN_NATIVE_TESTS_DIR}/" OUTPUT_VARIABLE
       test_sources)
  file(GLOB internal_support CONFIGURE_DEPENDS
       "${MLN_NATIVE_TESTS_DIR}/internal/support/*.cpp")
  add_executable(mln_native_internal_tests ${test_sources} ${internal_support})
  mln_native_configure_suite(mln_native_internal_tests "${registry_dir}")
  # The suite shares the library's C++ types, such as std::any, so it builds
  # without RTTI as the library does.
  mln_ffi_configure_c_api_compile_options(mln_native_internal_tests)
  target_compile_definitions(
    mln_native_internal_tests
    PRIVATE $<TARGET_PROPERTY:maplibre_native_c_objects,COMPILE_DEFINITIONS>)
  target_include_directories(
    mln_native_internal_tests
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
      $<TARGET_PROPERTY:maplibre_native_c_objects,INCLUDE_DIRECTORIES>)
  target_include_directories(
    mln_native_internal_tests
    SYSTEM
    PRIVATE
      $<TARGET_PROPERTY:maplibre_native_c_objects,SYSTEM_INCLUDE_DIRECTORIES>)
  target_link_libraries(mln_native_internal_tests PRIVATE ${library})
  set(${out_sources} ${sources} PARENT_SCOPE)
endfunction()

# Each exit program makes its calls and returns from main at once, so the
# process exit that follows is the check. They link the shipped library, as
# the ABI suite does, and a program whose name ends in a backend tag builds
# only on matching presets, as a suite file does.
function(mln_native_add_exit_tests out_tests)
  mln_native_active_file_tags(active_tags)
  file(GLOB programs RELATIVE "${MLN_NATIVE_TESTS_DIR}/exit" CONFIGURE_DEPENDS
       "${MLN_NATIVE_TESTS_DIR}/exit/*.c")
  if(NOT WIN32)
    find_package(Threads REQUIRED)
  endif()
  set(tests)
  foreach(program IN LISTS programs)
    mln_native_file_selected("${program}" "${active_tags}" selected)
    if(NOT selected)
      continue()
    endif()
    get_filename_component(stem "${program}" NAME_WE)
    set(target "mln_native_exit_${stem}")
    add_executable(${target} "${MLN_NATIVE_TESTS_DIR}/exit/${program}")
    set_target_properties(
      ${target}
      PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)
    target_link_libraries(${target} PRIVATE maplibre_native_c)
    if(NOT WIN32)
      target_link_libraries(${target} PRIVATE Threads::Threads)
    endif()
    if(TARGET mln_test_graphics_objects)
      target_link_libraries(${target} PRIVATE mln_test_graphics_objects)
    endif()
    add_test(NAME "native-exit/${stem}" COMMAND ${target})
    set_tests_properties("native-exit/${stem}" PROPERTIES TIMEOUT 60)
    list(APPEND tests "native-exit/${stem}")
  endforeach()
  set(${out_tests} ${tests} PARENT_SCOPE)
endfunction()

function(mln_ffi_add_native_tests)
  mln_ffi_add_test_graphics()
  get_target_property(test_supported mln_ffi_platform_dependencies
                      MLN_FFI_TEST_SUPPORTED)
  if(NOT test_supported)
    return()
  endif()

  mln_native_fetch_unity()
  mln_native_add_test_plugin()
  mln_native_add_abi_suite(abi_sources)
  mln_native_add_internal_suite(internal_sources)

  if(EMSCRIPTEN)
    mln_native_add_browser_abi_shards()
    mln_native_add_browser_test(native-internal/browser
                                mln_native_internal_tests)
    return()
  endif()

  # Each registered test gets the same environment, so a new entry cannot miss
  # the fixture directory or the loader path.
  mln_native_test_environment(test_environment test_modifications)
  set(registered_tests)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS)$" AND NOT maccatalyst)
    # A simulator spawns one process per run, so each suite runs as one entry,
    # with the plugin group in an invocation of its own after the ABI suite.
    # Each runner's alarm stays below its entry's timeout, so the runner
    # reports first.
    set(script "${PROJECT_SOURCE_DIR}/scripts/run-ios-simulator-test.sh")
    set(runner bash "${script}" "$<TARGET_FILE:mln_native_abi_tests>")
    add_test(NAME native-abi COMMAND ${runner} 290 -- -x /abi/plugin/)
    add_test(NAME native-abi/plugin COMMAND ${runner} 110 -- -f /abi/plugin/)
    add_test(
      NAME native-internal
      COMMAND bash "${script}" "$<TARGET_FILE:mln_native_internal_tests>" 110)
    set_tests_properties(native-abi PROPERTIES TIMEOUT 300)
    set_tests_properties(
      native-abi/plugin
      PROPERTIES DEPENDS native-abi TIMEOUT 120)
    set_tests_properties(native-internal PROPERTIES TIMEOUT 120)
    list(APPEND registered_tests native-abi native-abi/plugin native-internal)
  else()
    # One entry per file, so `ctest --parallel` spreads the suites across
    # processes and a crash takes down one file rather than the run.
    foreach(relative IN LISTS abi_sources internal_sources)
      string(REGEX MATCH "^[a-z]+" suite "${relative}")
      string(REGEX REPLACE "^[a-z]+/(.*)\\.[a-z]+$" "\\1" test_path
             "${relative}")
      set(test_name "native-${suite}/${test_path}")
      add_test(
        NAME ${test_name}
        COMMAND mln_native_${suite}_tests -f "/${relative}")
      set_tests_properties(${test_name} PROPERTIES TIMEOUT 120)
      list(APPEND registered_tests ${test_name})
    endforeach()
    mln_native_add_exit_tests(exit_tests)
    list(APPEND registered_tests ${exit_tests})
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
