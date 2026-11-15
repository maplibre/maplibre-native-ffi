# Builds the native suites under tests/native and registers how each target
# runs them. tests/native/support/test_support.h states how a test file is laid
# out and the rules that cases follow.
#
# The suites are compiled once for each backend in MLN_FFI_TEST_BACKENDS, as
# mln_native_abi_tests_<backend>, and every one links the same library. Each
# registered test carries its backend as a CTest label and as
# MLN_FFI_TEST_BACKEND in its environment, so `ctest -L <backend>` runs one
# backend's suites.

include(mln_ffi_test_graphics)

set(MLN_NATIVE_TESTS_DIR "${PROJECT_SOURCE_DIR}/tests/native")

# The ABI suite's domains, one directory each under tests/native/abi, in the
# order the registry runs them.
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
    platform)

# Tags a test file can end its name with to build only on matching targets,
# such as backend/single_context_device_egl.c.
set(MLN_NATIVE_FILE_TAGS
    metal
    opengl
    vulkan
    egl
    wgl
    android
    ohos
    apple
    wayland
    linux
    windows
    icu)

function(mln_native_active_file_tags out_var backend)
  set(tags ${backend})
  if(backend STREQUAL "opengl")
    list(APPEND tags ${MLN_FFI_OPENGL_CONTEXT_PROVIDER})
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

# Collects the ABI suite's test files for this target and a backend, in
# registry order, and the group symbol each one defines.
function(mln_native_collect_abi_tests out_sources out_symbols backend)
  mln_native_active_file_tags(active_tags ${backend})
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
    # Every backend's suite compiles the file, with the same group name.
    set(source "${MLN_NATIVE_TESTS_DIR}/${relative}")
    get_source_file_property(definitions "${source}" COMPILE_DEFINITIONS)
    if(NOT "MLN_TEST_GROUP_NAME=${symbol}" IN_LIST definitions)
      set_property(
        SOURCE "${source}"
        APPEND
        PROPERTY COMPILE_DEFINITIONS "MLN_TEST_GROUP_NAME=${symbol}")
    endif()
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

# The environment of a backend's tests.
function(mln_native_test_environment out_environment out_modifications backend)
  mln_ffi_render_dependency_property(dependency_runtime_dirs ${backend}
                                     MLN_FFI_RUNTIME_DIRS)
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
  set(environment "MLN_FFI_TEST_FIXTURE_DIR=${MLN_NATIVE_TESTS_DIR}/fixtures"
      "MLN_FFI_TEST_BACKEND=${backend}")
  set(${out_environment} ${environment} PARENT_SCOPE)
  set(${out_modifications} ${modifications} PARENT_SCOPE)
endfunction()

function(mln_native_configure_suite target registry_dir backend)
  mln_ffi_render_dependency_property(dependency_include_dirs ${backend}
                                     MLN_FFI_INCLUDE_DIRS)
  # Libraries the harness links for the graphics API it drives itself, which
  # the C API resolves at runtime rather than linking.
  mln_ffi_render_dependency_property(dependency_test_libraries ${backend}
                                     MLN_FFI_TEST_LINK_LIBRARIES)

  set(support_dir "${MLN_NATIVE_TESTS_DIR}/support")
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
      "${support_dir}/render_graphics.c")
  target_sources(${target} PRIVATE "${support_dir}/http_server.c")
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
      unity::framework mln_ffi_render_dependencies_${backend}
      ${dependency_test_libraries})
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

  if(backend STREQUAL "metal")
    target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_BACKEND_METAL=1)
  elseif(backend STREQUAL "opengl")
    target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_BACKEND_OPENGL=1)
    if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
      target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_OPENGL_WGL=1)
    else()
      target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_OPENGL_EGL=1)
    endif()
  elseif(backend STREQUAL "vulkan")
    target_compile_definitions(${target} PRIVATE MLN_FFI_TEST_BACKEND_VULKAN=1)
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
  if(APPLE AND backend STREQUAL "metal")
    target_link_libraries(
      ${target}
      PRIVATE "-framework Foundation" "-framework IOSurface" objc)
  endif()

  get_target_property(test_link_options mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LINK_OPTIONS)
  if(test_link_options)
    target_link_options(${target} PRIVATE ${test_link_options})
  endif()
endfunction()

# The ABI suite includes public headers only and links the shipped library,
# the shared one where the platform builds it, so it exercises the export
# boundary hosts link against. Each backend's suite is its own executable.
function(mln_native_add_abi_suite out_sources backend)
  mln_native_collect_abi_tests(sources symbols ${backend})
  set(registry_dir "${CMAKE_CURRENT_BINARY_DIR}/tests/native/abi/${backend}")
  mln_native_write_registry("${registry_dir}" "${sources}" "${symbols}")
  list(TRANSFORM sources PREPEND "${MLN_NATIVE_TESTS_DIR}/" OUTPUT_VARIABLE
       test_sources)
  set(target mln_native_abi_tests_${backend})
  add_executable(${target} ${test_sources})
  mln_native_configure_suite(${target} "${registry_dir}" ${backend})
  target_link_libraries(${target} PRIVATE maplibre_native_c)
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
  if(NOT CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS|visionOS)$"
     AND NOT MLN_FFI_TARGET_ARCHITECTURE MATCHES "^(riscv64|loongarch64)$")
    set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK device)
  endif()
  set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT ${environment})
endfunction()

# Each exit program makes its calls and returns from main at once, so the
# process exit that follows is the check. They link the shipped library, as
# the ABI suite does, and a program whose name ends in a backend tag builds
# only for a matching backend, as a suite file does. A program without one
# builds once and runs with each backend's tests.
function(mln_native_add_exit_tests out_tests backend)
  mln_native_active_file_tags(active_tags ${backend})
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
    set(test_name "native-exit/${backend}/${stem}")
    if(TARGET ${target})
      add_test(NAME ${test_name} COMMAND ${target})
      mln_native_bound_test(${test_name} ${target} 60)
      list(APPEND tests ${test_name})
      continue()
    endif()
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
    add_test(NAME ${test_name} COMMAND ${target})
    mln_native_bound_test(${test_name} ${target} 60)
    list(APPEND tests ${test_name})
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
  mln_native_add_doc_snippets()
  foreach(backend IN LISTS MLN_FFI_TEST_BACKENDS)
    mln_native_add_backend_tests(${backend})
  endforeach()
endfunction()

# Builds and registers one backend's suites.
function(mln_native_add_backend_tests backend)
  mln_native_add_abi_suite(abi_sources ${backend})
  set(abi_target mln_native_abi_tests_${backend})

  # Each registered test gets the same environment, so a new entry cannot miss
  # the fixture directory or the loader path.
  mln_native_test_environment(test_environment test_modifications ${backend})
  set(registered_tests)
  mln_native_runs_off_host(off_host)
  if(off_host)
    # Off the host, each suite runs as one entry, since a simulator spawns and
    # an emulator receives one process per run.
    set(test_name "native-abi/${backend}")
    add_test(NAME ${test_name} COMMAND ${abi_target})
    mln_native_bound_test(${test_name} ${abi_target} 300)
    list(APPEND registered_tests ${test_name})
    if(NOT CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS|visionOS)$")
      mln_native_add_exit_tests(exit_tests ${backend})
      list(APPEND registered_tests ${exit_tests})
    endif()
  else()
    # One entry per file, so `ctest --parallel` spreads the suites across
    # processes and a crash takes down one file rather than the run.
    foreach(relative IN LISTS abi_sources)
      string(REGEX MATCH "^[a-z]+" suite "${relative}")
      string(REGEX REPLACE "^[a-z]+/(.*)\\.[a-z]+$" "\\1" test_path
             "${relative}")
      set(test_name "native-${suite}/${backend}/${test_path}")
      add_test(
        NAME ${test_name}
        COMMAND mln_native_${suite}_tests_${backend} -f "/${relative}")
      mln_native_bound_test(${test_name} mln_native_${suite}_tests_${backend}
                            120)
      list(APPEND registered_tests ${test_name})
    endforeach()
    mln_native_add_exit_tests(exit_tests ${backend})
    list(APPEND registered_tests ${exit_tests})
  endif()
  foreach(test_name IN LISTS registered_tests)
    set_property(TEST ${test_name} PROPERTY LABELS ${backend})
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
