# The browser module of the Kotlin/Wasm binding.
#
# The presets include the root presets, so a preset name selects the same
# Emscripten toolchain as the native build. The module links the archives and
# link flags from that build's install prefix with the generated wrappers and
# the binding's wake shim, into an ES module factory and its .wasm. Kotlin
# bundles the factory through the selected runtime KLIB.
cmake_minimum_required(VERSION 3.28)
project(maplibre_native_kotlin_wasm LANGUAGES C)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "The Kotlin/Wasm module builds with an Emscripten preset")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

get_filename_component(
  MLN_FFI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.."
  ABSOLUTE)
set(MLN_FFI_NATIVE_PREFIX
    "${MLN_FFI_ROOT}/build/${MLN_FFI_ARTIFACT_NAME}/install"
    CACHE PATH "Install prefix of the native build that the module links")

find_library(
  MLN_FFI_NATIVE_ARCHIVE
  NAMES libmaplibre-native-c.a
  PATHS "${MLN_FFI_NATIVE_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
  REQUIRED)
# The archives that ship beside the complete one, such as the Rust platform
# layer.
file(GLOB MLN_FFI_COMPANION_ARCHIVES
     "${MLN_FFI_NATIVE_PREFIX}/lib/libmln_ffi_*.a")
# The settings that every module linking the archives must share, such as
# pthreads, the heap size, and the exception model.
set(MLN_FFI_LINK_FLAGS_FILE
    "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/emscripten-link-flags.txt")
file(STRINGS "${MLN_FFI_LINK_FLAGS_FILE}" MLN_FFI_LINK_FLAGS)
list(FILTER MLN_FFI_LINK_FLAGS EXCLUDE REGEX "^[ \t]*$")
# loadWasmModule passes the pool size that its caller asks for as
# Module.mlnPoolSize, which the module reads as it starts. The installed size
# stays the default for a caller that passes none.
set(MLN_KW_POOL_DEFAULT "")
foreach(flag IN LISTS MLN_FFI_LINK_FLAGS)
  if(flag MATCHES "^-sPTHREAD_POOL_SIZE=([0-9]+)$")
    set(MLN_KW_POOL_DEFAULT "${CMAKE_MATCH_1}")
  endif()
endforeach()
if(MLN_KW_POOL_DEFAULT STREQUAL "")
  message(
    FATAL_ERROR "${MLN_FFI_LINK_FLAGS_FILE} sets no numeric PTHREAD_POOL_SIZE")
endif()
list(FILTER MLN_FFI_LINK_FLAGS EXCLUDE REGEX "^-sPTHREAD_POOL_SIZE=")
list(APPEND MLN_FFI_LINK_FLAGS
     "-sPTHREAD_POOL_SIZE=Module.mlnPoolSize||${MLN_KW_POOL_DEFAULT}")

set(MLN_KW_EXPORTS "${CMAKE_CURRENT_SOURCE_DIR}/mln_kw_exports.txt")
set(MLN_KW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/mln_kw_library.js")

# WebGraphics reaches the graphics library of the build's backend as a
# property of the module, which imports a page's objects into the module's
# tables.
set(MLN_FFI_ARTIFACT_FILE
    "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/artifact.json")
file(READ "${MLN_FFI_ARTIFACT_FILE}" MLN_FFI_ARTIFACT)
string(JSON MLN_FFI_RENDER_BACKEND GET "${MLN_FFI_ARTIFACT}" backend)
if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
  set(MLN_KW_GRAPHICS_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/mln_kw_webgpu.js")
  set(MLN_KW_GRAPHICS_OBJECT mlnKwWebGpu)
elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
  set(MLN_KW_GRAPHICS_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/mln_kw_webgl.js")
  set(MLN_KW_GRAPHICS_OBJECT mlnKwWebGl)
else()
  message(
    FATAL_ERROR
      "${MLN_FFI_ARTIFACT_FILE} names render backend ${MLN_FFI_RENDER_BACKEND}, "
      "and a browser module renders with webgpu or opengl")
endif()

# Links the module [target] as [output].mjs from the binding's sources and its
# graphics library, the C files in SOURCES, and the JavaScript libraries in
# JS_LIBRARIES.
function(mln_kw_module target output)
  cmake_parse_arguments(PARSE_ARGV 2 MODULE "" "" "SOURCES;JS_LIBRARIES")
  add_executable(
    ${target} mln_kw_generated.c mln_kw_runtime.c ${MODULE_SOURCES})
  set_target_properties(
    ${target}
    PROPERTIES OUTPUT_NAME ${output} SUFFIX ".mjs")
  target_include_directories(
    ${target}
    PRIVATE "${MLN_FFI_NATIVE_PREFIX}/include" "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(${target} PRIVATE MLN_STATIC)
  target_compile_options(
    ${target}
    PRIVATE -pthread -fwasm-exceptions -Wall -Wextra -Wno-unused-parameter)
  target_link_libraries(
    ${target}
    PRIVATE "${MLN_FFI_NATIVE_ARCHIVE}" ${MLN_FFI_COMPANION_ARCHIVES})
  set(js_libraries "${MLN_KW_LIBRARY}" "${MLN_KW_GRAPHICS_LIBRARY}"
      ${MODULE_JS_LIBRARIES})
  list(TRANSFORM js_libraries PREPEND "--js-library=" OUTPUT_VARIABLE
       js_library_flags)
  target_link_options(
    ${target}
    PRIVATE
      ${MLN_FFI_LINK_FLAGS}
      -sMODULARIZE
      -sEXPORT_ES6
      -sEXPORT_NAME=createMaplibreNativeKotlin
      -sENVIRONMENT=web,worker
      # A failed allocation returns null to the caller, which reports it.
      -sABORTING_MALLOC=0
      # A transferred-canvas WebGL session hands its canvas to the core worker
      # that pthread_create starts. No thread takes the default list, which
      # would name a #canvas that the page need not have.
      -sOFFSCREENCANVAS_SUPPORT=1
      "-sOFFSCREENCANVASES_TO_PTHREAD=''"
      "-sEXPORTED_FUNCTIONS=@${MLN_KW_EXPORTS}"
      -sEXPORTED_RUNTIME_METHODS=HEAPU8,${MLN_KW_GRAPHICS_OBJECT}
      ${js_library_flags})
  set_property(
    TARGET ${target}
    APPEND
    PROPERTY
      LINK_DEPENDS "${MLN_KW_EXPORTS}" ${js_libraries}
      "${MLN_FFI_LINK_FLAGS_FILE}")
  install(
    FILES "$<TARGET_FILE:${target}>"
    "$<TARGET_FILE_DIR:${target}>/${output}.wasm"
    DESTINATION .
    COMPONENT ${target})
endfunction()

mln_kw_module(maplibre_native_kotlin maplibre-native-kotlin)
include("${MLN_FFI_ROOT}/cmake/mln_ffi_binding_sbom.cmake")
mln_ffi_install_wasm_sbom(maplibre_native_kotlin)
# The module that the Kotlin/Wasm suite loads, with the fixtures in
# test_support.c and their canvases and pool probe in test_support.js. The page
# creates its own WebGL contexts and WebGPU devices, which the suite imports
# through WebGraphics.
mln_kw_module(
  maplibre_native_kotlin_test maplibre-native-kotlin-test SOURCES test_support.c
  JS_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/test_support.js")
