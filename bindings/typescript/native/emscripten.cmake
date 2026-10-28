# The browser module of the TypeScript binding, which CMakeLists.txt includes
# for an Emscripten preset.
#
# A browser has no Node-API host, so the module links emnapi's implementation
# of Node-API with the addon's sources and the archives and link flags of the
# preset's install prefix, into an ES module factory and its .wasm. The
# binding's browser entry loads the factory.

set(MLN_FFI_EMNAPI "${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/emnapi"
    CACHE PATH "The emnapi package")
if(NOT EXISTS "${MLN_FFI_EMNAPI}/CMakeLists.txt")
  message(
    FATAL_ERROR
      "emnapi not found in ${MLN_FFI_EMNAPI}; "
      "install the TypeScript binding's packages first")
endif()
# emnapi builds from source with this toolchain, so its archive shares the
# module's pthreads ABI. Its libuv sources call POSIX functions that strict C11
# hides, and test Emscripten's deprecated version macros.
block()
  set(CMAKE_C_EXTENSIONS ON)
  string(APPEND CMAKE_C_FLAGS " -Wno-deprecated-pragma")
  add_subdirectory("${MLN_FFI_EMNAPI}" emnapi EXCLUDE_FROM_ALL)
endblock()

# The settings that every module linking the archives must share, such as
# pthreads, the heap size, and the exception model.
set(MLN_FFI_LINK_FLAGS_FILE
    "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/emscripten-link-flags.txt")
file(STRINGS "${MLN_FFI_LINK_FLAGS_FILE}" MLN_FFI_LINK_FLAGS)
list(FILTER MLN_FFI_LINK_FLAGS EXCLUDE REGEX "^[ \t]*$")
# The browser entry passes the pool size that its caller asks for as
# Module.mlnPoolSize, which the module reads as it starts. The installed size
# stays the default for a caller that passes none.
set(MLN_FFI_POOL_DEFAULT "")
foreach(flag IN LISTS MLN_FFI_LINK_FLAGS)
  if(flag MATCHES "^-sPTHREAD_POOL_SIZE=([0-9]+)$")
    set(MLN_FFI_POOL_DEFAULT "${CMAKE_MATCH_1}")
  endif()
endforeach()
if(MLN_FFI_POOL_DEFAULT STREQUAL "")
  message(
    FATAL_ERROR "${MLN_FFI_LINK_FLAGS_FILE} sets no numeric PTHREAD_POOL_SIZE")
endif()
list(FILTER MLN_FFI_LINK_FLAGS EXCLUDE REGEX "^-sPTHREAD_POOL_SIZE=")
list(APPEND MLN_FFI_LINK_FLAGS
     "-sPTHREAD_POOL_SIZE=Module.mlnPoolSize||${MLN_FFI_POOL_DEFAULT}")

# What emnapi's loader calls: its entry points, and the allocator that its
# JavaScript half writes values through.
set(MLN_FFI_EMNAPI_EXPORTS
    _malloc
    _free
    _napi_register_wasm_v1
    _node_api_module_get_api_version_v1
    _emnapi_create_env
    _emnapi_delete_env
    _uv_library_shutdown)
list(JOIN MLN_FFI_EMNAPI_EXPORTS "," MLN_FFI_EMNAPI_EXPORTS)

# The graphics imports of the browser entry reach the library of the build's
# backend as a property of the module, which imports a page's objects into the
# module's tables.
file(READ "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/artifact.json"
     MLN_FFI_ARTIFACT)
string(JSON MLN_FFI_RENDER_BACKEND GET "${MLN_FFI_ARTIFACT}" backend)
if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
  set(MLN_FFI_GRAPHICS_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/mln_ts_webgpu.js")
  set(MLN_FFI_GRAPHICS_OBJECT mlnTsWebGpu)
elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
  set(MLN_FFI_GRAPHICS_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/mln_ts_webgl.js")
  set(MLN_FFI_GRAPHICS_OBJECT mlnTsWebGl)
else()
  message(
    FATAL_ERROR
      "the native build renders with ${MLN_FFI_RENDER_BACKEND}, "
      "and a browser module renders with webgpu or opengl")
endif()

# Links [target] from the addon's sources, the archives, and emnapi into an ES
# module factory and its .wasm.
function(mln_ffi_browser_module target)
  set_target_properties(${target} PROPERTIES SUFFIX ".mjs")
  target_include_directories(
    ${target}
    PRIVATE
      "${MLN_FFI_ROOT}/include" "${MLN_FFI_NATIVE_PREFIX}/include"
      "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(${target} PRIVATE MLN_STATIC)
  target_compile_options(
    ${target}
    PRIVATE -pthread -fwasm-exceptions -Wall -Wextra -Wno-unused-parameter)
  target_link_libraries(
    ${target}
    PRIVATE "${MLN_FFI_NATIVE_ARCHIVE}" ${MLN_FFI_COMPANION_ARCHIVES} emnapi-mt)
  target_link_options(
    ${target}
    PRIVATE
      ${MLN_FFI_LINK_FLAGS}
      -sMODULARIZE
      -sEXPORT_ES6
      -sENVIRONMENT=web,worker
      # A failed allocation returns null to the caller, which reports it.
      -sABORTING_MALLOC=0
      # A transferred-canvas WebGL session hands its canvas to the core worker
      # that pthread_create starts. No thread takes the default list, which
      # would name a #canvas that the page need not have.
      -sOFFSCREENCANVAS_SUPPORT=1
      "-sOFFSCREENCANVASES_TO_PTHREAD=''"
      "-sEXPORTED_FUNCTIONS=${MLN_FFI_EMNAPI_EXPORTS}"
      -sEXPORTED_RUNTIME_METHODS=${MLN_FFI_GRAPHICS_OBJECT}
      "--js-library=${MLN_FFI_GRAPHICS_LIBRARY}")
  set_property(
    TARGET ${target}
    APPEND
    PROPERTY
      LINK_DEPENDS "${MLN_FFI_LINK_FLAGS_FILE}" "${MLN_FFI_GRAPHICS_LIBRARY}")
endfunction()

add_executable(maplibre_native_ffi ${MLN_FFI_SOURCES})
mln_ffi_browser_module(maplibre_native_ffi)
install(
  FILES "$<TARGET_FILE:maplibre_native_ffi>"
  "$<TARGET_FILE_DIR:maplibre_native_ffi>/maplibre_native_ffi.wasm"
  DESTINATION .)
# The addon package ships the licenses of the native library it embeds, as the
# desktop addon's does.
install(
  DIRECTORY "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/licenses/"
  DESTINATION licenses)

# The module that the browser suite loads, whose entry adds the test exports
# beside the addon's.
add_executable(
  maplibre_native_ffi_test
  EXCLUDE_FROM_ALL
    ${MLN_FFI_SOURCES} ../test/browser/test_addon.c
    ../test/support/native_probe.c)
mln_ffi_browser_module(maplibre_native_ffi_test)
target_include_directories(
  maplibre_native_ffi_test
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../test/support")
