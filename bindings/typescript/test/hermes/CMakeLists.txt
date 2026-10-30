cmake_minimum_required(VERSION 3.28)
project(maplibre_native_ffi_hermes_test LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_BUNDLE OFF)
get_filename_component(
  MLN_FFI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.."
  ABSOLUTE)
if(NOT MLN_TS_NATIVE)
  get_filename_component(
    MLN_TS_NATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../../native"
    ABSOLUTE)
endif()
set(MLN_FFI_NODE_API_HEADERS "${MLN_TS_NATIVE}/node-api-headers")
if(NOT EXISTS "${MLN_FFI_NODE_API_HEADERS}/node_api.h")
  set(MLN_FFI_NODE_API_HEADERS
      "${MLN_TS_NATIVE}/../node_modules/node-api-headers/include")
endif()
if(ANDROID)
  set(MLN_HERMES_INCLUDE
      "${MLN_HERMES_ARTIFACT}/prefab/modules/hermesvm/include")
  set(MLN_HERMES_LIBRARY
      "${MLN_HERMES_ARTIFACT}/jni/${ANDROID_ABI}/libhermesvm.so")
elseif(APPLE)
  set(MLN_HERMES_INCLUDE "${MLN_HERMES_ARTIFACT}/destroot/include")
  set(MLN_HERMES_LIBRARY "${MLN_HERMES_APPLE_FRAMEWORK}")
endif()
if(APPLE)
  # The official Apple framework contains JSI as well as the VM.
  set(MLN_HERMES_JSI_DIR "${MLN_HERMES_INCLUDE}")
  add_library(jsi INTERFACE)
  target_include_directories(jsi INTERFACE "${MLN_HERMES_JSI_DIR}")
  target_link_libraries(jsi INTERFACE "${MLN_HERMES_LIBRARY}")
else()
  # Android's official VM depends on a separate, matching JSI shared library.
  set(MLN_HERMES_JSI_DIR "${MLN_HERMES_SOURCE}/API/jsi")
  set(BUILD_SHARED_LIBS ${ANDROID})
  add_subdirectory(
    "${MLN_HERMES_JSI_DIR}/jsi" "${CMAKE_CURRENT_BINARY_DIR}/jsi")
endif()
add_subdirectory(
  "${MLN_TS_NATIVE}/hermes" "${CMAKE_CURRENT_BINARY_DIR}/binding")

foreach(bundle suite smoke)
  file(READ "${MLN_HERMES_TEST_${bundle}}" bundle_bytes HEX)
  string(REGEX REPLACE "(..)" "0x\\1," bundle_bytes "${bundle_bytes}")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${bundle}.h"
       "static const unsigned char ${bundle}[] = {${bundle_bytes}};\n")
endforeach()
set(test_sources
    native/main.cpp native/Probe.cpp native/shim_test.c native/test_graphics.c
    ../support/native_probe.c "${MLN_FFI_ROOT}/tests/graphics/graphics.c")
if(ANDROID)
  add_library(hermes-test SHARED ${test_sources})
else()
  add_executable(hermes-test ${test_sources})
endif()
target_include_directories(
  hermes-test
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${MLN_HERMES_INCLUDE}"
    "${MLN_FFI_NATIVE_PREFIX}/include"
    "${MLN_FFI_NODE_API_HEADERS}"
    "${MLN_FFI_ROOT}/tests/graphics/include"
    "${MLN_FFI_ROOT}/third_party/maplibre-native/vendor/Vulkan-Headers/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../support")
target_compile_options(
  hermes-test
  PRIVATE -include "${MLN_TS_NATIVE}/hermes/symbols.h")
target_link_libraries(
  hermes-test
  PRIVATE maplibre_native_ffi_hermes "${MLN_HERMES_LIBRARY}")
if(ANDROID)
  target_include_directories(
    hermes-test
    PRIVATE "${MLN_HERMES_FBJNI}/prefab/modules/fbjni/include")
  target_link_libraries(
    hermes-test
    PRIVATE log android dl "${MLN_HERMES_FBJNI}/jni/${ANDROID_ABI}/libfbjni.so")
else()
  target_link_options(
    hermes-test
    PRIVATE "LINKER:-rpath,${MLN_FFI_NATIVE_PREFIX}/lib")
endif()
