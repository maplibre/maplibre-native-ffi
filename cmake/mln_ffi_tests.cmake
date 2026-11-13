# Builds the native suites under tests/native and registers how each target
# runs them. tests/native/support/test_support.h states how a test file is laid
# out and the rules that cases follow.

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
    delivery
    binding
    platform
    plugin)

# The browser runs the ABI suite as four pages rather than one per file, which
# would launch Chromium once per file. Each shard names its domains.
set(MLN_NATIVE_BROWSER_SHARDS
    "base,completion,runtime" "resources,map,projection,style" "render,backend"
    "delivery,binding,platform,plugin")

# Tags a test file can end its name with to build only on matching targets,
# such as backend/single_context_device_egl.c or
# platform/browser_http_emscripten.c.
set(MLN_NATIVE_FILE_TAGS
    metal
    opengl
    vulkan
    webgpu
    egl
    wgl
    webgl
    emscripten
    android
    ohos
    apple
    wayland
    linux
    windows
    icu)

function(mln_native_active_file_tags out_var)
  set(tags ${MLN_FFI_RENDER_BACKEND})
  if(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    list(APPEND tags ${MLN_FFI_OPENGL_CONTEXT_PROVIDER})
  endif()
  if(EMSCRIPTEN)
    list(APPEND tags emscripten)
  endif()
  if(ANDROID)
    list(APPEND tags android)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    list(APPEND tags ohos)
  endif()
  if(APPLE)
    list(APPEND tags apple)
  endif()
  if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD)$")
    list(APPEND tags wayland)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND tags linux)
  endif()
  if(WIN32)
    list(APPEND tags windows)
  endif()
  # Linux, FreeBSD, OpenHarmony, and Windows evaluate locale expressions with
  # ICU: ICU4X on Linux and FreeBSD and the system's ICU4C on OpenHarmony and
  # Windows.
  if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD|OHOS|Windows)$")
    list(APPEND tags icu)
  endif()
  set(${out_var} ${tags} PARENT_SCOPE)
endfunction()

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

# The cases and programs that a preset expects to fail, because the library
# doesn't do yet what they test. tests/native/expected-failures/ holds a list
# for each backend, named by the backend or, for OpenGL, its context provider
# (vulkan.txt, egl.txt, metal.txt), and optionally one for a system and backend
# (android-vulkan.txt). A line names a suite case as <file>:<case>, or an exit
# program as its file, relative to tests/native, followed by `  # ` and why it
# fails. The harness reports a listed case that fails as ignored, and CTest
# expects a listed program to fail. Either fails when what it lists passes, so
# a list only ever shrinks where the review sees it. A case that takes the
# process down with it, which would end the cases after it, starts its line
# with `!`: the harness skips it instead of running it.
#
# Sets `out_cases` to the harness's entries, one MLN_TEST_EXPECTED_FAILURE()
# line each, and `out_programs` to the listed exit programs' stems.
function(mln_native_read_expected_failures out_cases out_programs)
  string(TOLOWER "${CMAKE_SYSTEM_NAME}" system)
  set(backend ${MLN_FFI_RENDER_BACKEND})
  if(backend STREQUAL "opengl")
    set(backend ${MLN_FFI_OPENGL_CONTEXT_PROVIDER})
  endif()
  set(cases "")
  set(programs "")
  foreach(name IN ITEMS "${backend}" "${system}-${backend}")
    set(list_file "${MLN_NATIVE_TESTS_DIR}/expected-failures/${name}.txt")
    set_property(
      DIRECTORY
      APPEND
      PROPERTY CMAKE_CONFIGURE_DEPENDS "${list_file}")
    if(NOT EXISTS "${list_file}")
      continue()
    endif()
    file(STRINGS "${list_file}" lines)
    foreach(line IN LISTS lines)
      string(STRIP "${line}" line)
      if(line STREQUAL "" OR line MATCHES "^#")
        continue()
      endif()
      if(NOT line MATCHES "^(!?)([^:! ]+)(:([A-Za-z0-9_]+))?( +# *(.*))?$")
        message(
          FATAL_ERROR "${list_file}: not [!]<file>[:<case>]  # reason: ${line}"
        )
      endif()
      set(skip 0)
      if(CMAKE_MATCH_1)
        set(skip 1)
      endif()
      set(file "${CMAKE_MATCH_2}")
      set(case "${CMAKE_MATCH_4}")
      set(reason "${CMAKE_MATCH_6}")
      # A list that names what the suite no longer has would hide nothing, and
      # would keep a stale entry around.
      if(NOT EXISTS "${MLN_NATIVE_TESTS_DIR}/${file}")
        message(FATAL_ERROR "${list_file} names ${file}, which doesn't exist")
      endif()
      if(NOT case)
        if(skip)
          message(FATAL_ERROR "${list_file}: only a case is skipped: ${line}")
        endif()
        if(NOT file MATCHES "^exit/([^/]+)\\.c$")
          message(
            FATAL_ERROR
              "${list_file} names ${file} without a case; only exit programs fail as a whole"
          )
        endif()
        list(APPEND programs "${CMAKE_MATCH_1}")
        continue()
      endif()
      file(
        STRINGS "${MLN_NATIVE_TESTS_DIR}/${file}" defined
        REGEX "RUN_TEST[(]${case}[),]")
      if(NOT defined)
        message(
          FATAL_ERROR "${list_file} names ${file}:${case}, which doesn't exist")
      endif()
      string(REPLACE "\\" "\\\\" reason "${reason}")
      string(REPLACE "\"" "\\\"" reason "${reason}")
      string(
        APPEND cases
        "MLN_TEST_EXPECTED_FAILURE(\"tests/native/${file}\", \"${case}\", \"${reason}\", ${skip})\n"
      )
    endforeach()
  endforeach()
  set(${out_cases} "${cases}" PARENT_SCOPE)
  set(${out_programs} ${programs} PARENT_SCOPE)
endfunction()

# Writes the harness's list of the cases that this preset expects to fail.
function(mln_native_write_expected_failures registry_dir)
  mln_native_read_expected_failures(cases programs)
  # Written as is: a reason may hold characters that file(CONFIGURE) expands.
  set(output "${registry_dir}/mln_native_expected_failures.inc")
  file(WRITE "${output}.tmp" "${cases}")
  file(COPY_FILE "${output}.tmp" "${output}" ONLY_IF_DIFFERENT)
  file(REMOVE "${output}.tmp")
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
    PRIVATE "${PROJECT_SOURCE_DIR}/include")
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
  # The suite stands in for a WebGPU host, and as written it waits in place:
  # for its own device, for buffer maps, and for sessions whose results arrive
  # on its event loop. So it suspends through JSPI. The library never does,
  # and links without it.
  if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
    target_link_options(${target} PRIVATE "-sJSPI")
  endif()
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
  get_target_property(vulkan_icd_file mln_ffi_render_dependencies
                      MLN_FFI_VULKAN_ICD_FILE)
  if(vulkan_icd_file)
    list(APPEND environment "VK_ICD_FILENAMES=${vulkan_icd_file}")
  endif()
  set(${out_environment} ${environment} PARENT_SCOPE)
  set(${out_modifications} ${modifications} PARENT_SCOPE)
endfunction()

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
  # The Wayland cases present to surfaces of a compositor that the suite runs
  # itself, on the libwayland that the graphics drivers bring.
  if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD)$")
    target_sources(${target} PRIVATE "${support_dir}/wayland_compositor.c")
    target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS})
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
    PRIVATE "${PROJECT_SOURCE_DIR}/include")

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
  # The Android cases run an ALooper of their own, as an app's main thread does,
  # and present to an AImageReader's window in place of an app's.
  if(ANDROID)
    target_link_libraries(${target} PRIVATE android mediandk)
  endif()
  # The OpenHarmony cases present to a consumer surface's window in place of an
  # app's, and read its buffers back.
  if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    target_link_libraries(
      ${target}
      PRIVATE native_buffer native_image native_window)
  endif()
  # The Metal test support defines an Objective-C class, which the iOS and
  # tvOS test app links against the runtime and Foundation directly. The
  # IOSurface cases lock surfaces to read them.
  if(APPLE AND MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    target_link_libraries(
      ${target}
      PRIVATE "-framework Foundation" "-framework IOSurface" objc)
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
  mln_native_write_expected_failures("${registry_dir}")
  list(TRANSFORM sources PREPEND "${MLN_NATIVE_TESTS_DIR}/" OUTPUT_VARIABLE
       test_sources)
  add_executable(mln_native_abi_tests ${test_sources})
  mln_native_configure_suite(mln_native_abi_tests "${registry_dir}")
  target_link_libraries(
    mln_native_abi_tests
    PRIVATE maplibre_native_c mln_native_test_plugin)
  set(${out_sources} ${sources} PARENT_SCOPE)
endfunction()

# Whether this target's executables run off the host: on a simulator, an
# Android or OpenHarmony emulator, in Alpine, in a FreeBSD virtual machine,
# under QEMU's user-mode emulator, or under Wine. Mac Catalyst runs on the host.
# Each of these targets sets CMAKE_SYSTEM_NAME, so a Windows host never runs a
# test through the dispatcher.
function(mln_native_runs_off_host out_var)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  set(off_host FALSE)
  if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS|visionOS)$" AND NOT maccatalyst)
    set(off_host TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "^(Android|OHOS|FreeBSD)$")
    set(off_host TRUE)
  elseif(MLN_FFI_ZIG_LIBC STREQUAL "musl"
         OR MLN_FFI_TARGET_ARCHITECTURE MATCHES "^(riscv64|loongarch64)$")
    set(off_host TRUE)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND NOT CMAKE_HOST_WIN32)
    set(off_host TRUE)
  endif()
  set(${out_var} ${off_host} PARENT_SCOPE)
endfunction()

# Bounds a test by its timeout. Off the host, CMake runs the target through
# tools/devices/run, its emulator, which reads this build's install to find the
# device. The dispatcher ends the executable 30 seconds before the timeout, so
# its runner reports and cleans up before CTest stops it. A simulator and QEMU
# run the suites side by side; any other device runs one test at a time.
function(mln_native_bound_test test_name target timeout)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT ${timeout})
  # On macOS, ANGLE's Metal backend can stop completing a process's GPU work
  # when several processes render at once on a virtual GPU, as CI runners have:
  # reads then return zeros or an earlier frame, with no GL error. EGL tests
  # there take turns. CMake can't tell which files render, so all of them do.
  if(APPLE AND MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK gpu)
  endif()
  mln_native_runs_off_host(off_host)
  if(NOT off_host)
    return()
  endif()
  set_target_properties(
    ${target}
    PROPERTIES CROSSCOMPILING_EMULATOR "${PROJECT_SOURCE_DIR}/tools/devices/run"
  )
  math(EXPR deadline "${timeout} - 30")
  set(environment "MLN_FFI_DEVICE_INSTALL=${CMAKE_INSTALL_PREFIX}"
      "MLN_FFI_DEVICE_TIMEOUT=${deadline}")
  # An emulator receives the plugin beside the executable.
  get_target_property(plugin_type mln_native_test_plugin TYPE)
  if(plugin_type STREQUAL "SHARED_LIBRARY")
    list(APPEND environment
         "MLN_FFI_DEVICE_LIBRARIES=$<TARGET_FILE:mln_native_test_plugin>")
  endif()
  if(NOT CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS|visionOS)$"
     AND NOT MLN_FFI_TARGET_ARCHITECTURE MATCHES "^(riscv64|loongarch64)$")
    set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK device)
  endif()
  set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT ${environment})
endfunction()

# Each exit program makes its calls and returns from main at once, so the
# process exit that follows is the check. They link the shipped library, as
# the ABI suite does, and a program whose name ends in a backend tag builds
# only on matching presets, as a suite file does.
function(mln_native_add_exit_tests out_tests)
  mln_native_active_file_tags(active_tags)
  mln_native_read_expected_failures(expected_cases expected_programs)
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
    mln_native_bound_test("native-exit/${stem}" ${target} 60)
    if(stem IN_LIST expected_programs)
      set_tests_properties("native-exit/${stem}" PROPERTIES WILL_FAIL TRUE)
    endif()
    list(APPEND tests "native-exit/${stem}")
  endforeach()
  set(${out_tests} ${tests} PARENT_SCOPE)
endfunction()

# Compiles the C snippets that the docs site shows, so they keep up with the
# headers. Nothing runs them.
function(mln_native_add_doc_snippets)
  file(GLOB sources CONFIGURE_DEPENDS "${MLN_NATIVE_TESTS_DIR}/docsnippets/*.c")
  add_library(mln_native_doc_snippets OBJECT ${sources})
  set_target_properties(
    mln_native_doc_snippets
    PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
  target_link_libraries(mln_native_doc_snippets PRIVATE maplibre_native_c)
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
  mln_native_add_doc_snippets()

  if(EMSCRIPTEN)
    mln_native_add_browser_abi_shards()
    return()
  endif()

  # Each registered test gets the same environment, so a new entry cannot miss
  # the fixture directory or the loader path.
  mln_native_test_environment(test_environment test_modifications)
  set(registered_tests)
  mln_native_runs_off_host(off_host)
  if(off_host)
    # Off the host, each suite runs as one entry, since a simulator spawns and
    # an emulator receives one process per run, with the plugin group in an
    # invocation of its own after the ABI suite.
    add_test(NAME native-abi COMMAND mln_native_abi_tests -x /abi/plugin/)
    mln_native_bound_test(native-abi mln_native_abi_tests 300)
    add_test(
      NAME native-abi/plugin
      COMMAND mln_native_abi_tests -f /abi/plugin/)
    mln_native_bound_test(native-abi/plugin mln_native_abi_tests 120)
    set_tests_properties(native-abi/plugin PROPERTIES DEPENDS native-abi)
    list(APPEND registered_tests native-abi native-abi/plugin)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS|visionOS)$")
      mln_native_add_exit_tests(exit_tests)
      list(APPEND registered_tests ${exit_tests})
    endif()
  else()
    # One entry per file, so `ctest --parallel` spreads the suites across
    # processes and a crash takes down one file rather than the run.
    foreach(relative IN LISTS abi_sources)
      string(REGEX MATCH "^[a-z]+" suite "${relative}")
      string(REGEX REPLACE "^[a-z]+/(.*)\\.[a-z]+$" "\\1" test_path
             "${relative}")
      set(test_name "native-${suite}/${test_path}")
      add_test(
        NAME ${test_name}
        COMMAND mln_native_${suite}_tests -f "/${relative}")
      mln_native_bound_test(${test_name} mln_native_${suite}_tests 120)
      list(APPEND registered_tests ${test_name})
    endforeach()
    mln_native_add_exit_tests(exit_tests)
    list(APPEND registered_tests ${exit_tests})
  endif()
  foreach(test_name IN LISTS registered_tests)
    set_property(
      TEST ${test_name}
      APPEND
      PROPERTY ENVIRONMENT ${test_environment})
    if(test_modifications)
      set_property(
        TEST ${test_name}
        PROPERTY ENVIRONMENT_MODIFICATION ${test_modifications})
    endif()
  endforeach()
endfunction()
