# The host supplies matching Hermes JSI headers and the JSI library. This
# adapter schedules all VM access through the callback passed to Module.
# Provide target jsi and MLN_HERMES_JSI_DIR for the matching engine headers,
# and MLN_FFI_NATIVE_PREFIX for an installed C library (or set
# MLN_FFI_HERMES_NATIVE_LIBRARY to a library or XCFramework explicitly).
cmake_minimum_required(VERSION 3.28)
project(maplibre_native_ffi_hermes LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(MLN_TS_NATIVE "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(MLN_FFI_NODE_API_HEADERS "${MLN_TS_NATIVE}/node-api-headers"
    CACHE PATH "Node-API headers")
if(NOT EXISTS "${MLN_FFI_NODE_API_HEADERS}/node_api.h")
  set(MLN_FFI_NODE_API_HEADERS
      "${MLN_TS_NATIVE}/../node_modules/node-api-headers/include")
endif()
find_library(
  MLN_FFI_HERMES_NATIVE_LIBRARY
  NAMES maplibre-native-c
  PATHS "${MLN_FFI_NATIVE_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
  REQUIRED)

add_library(
  maplibre_native_ffi_hermes
  STATIC
    "${MLN_TS_NATIVE}/generated_addon.c" "${MLN_TS_NATIVE}/generated_values.c"
    "${MLN_TS_NATIVE}/realm.c" "${MLN_TS_NATIVE}/runtime.c" shim.cpp module.cpp)
target_include_directories(
  maplibre_native_ffi_hermes
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${MLN_HERMES_JSI_DIR}"
  PRIVATE
    "${MLN_TS_NATIVE}" "${MLN_TS_NATIVE}/include"
    "${MLN_FFI_NATIVE_PREFIX}/include" "${MLN_FFI_NODE_API_HEADERS}")
set_target_properties(
  maplibre_native_ffi_hermes
  PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(
  maplibre_native_ffi_hermes
  PRIVATE
    -include "${CMAKE_CURRENT_SOURCE_DIR}/symbols.h" -Wall -Wextra
    -Wno-unused-parameter)
target_compile_definitions(maplibre_native_ffi_hermes PRIVATE NAPI_VERSION=8)
target_link_libraries(
  maplibre_native_ffi_hermes
  PUBLIC "${MLN_FFI_HERMES_NATIVE_LIBRARY}" jsi)
