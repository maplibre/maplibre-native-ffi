cmake_minimum_required(VERSION 3.22)
project(maplibre_native_ffi_react LANGUAGES C CXX)

get_filename_component(
  MLN_TS_NATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../native"
  ABSOLUTE)
set(MLN_RN_NATIVE "${MLN_TS_NATIVE}/react-native")
find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

find_package(maplibre-native-c REQUIRED CONFIG)

add_library(
  maplibre_native_ffi_react
  SHARED
    src/main/cpp/adapter.cpp
    ${MLN_RN_NATIVE}/shim.cpp
    ${MLN_RN_NATIVE}/module.cpp
    ${MLN_TS_NATIVE}/generated_addon.c
    ${MLN_TS_NATIVE}/generated_values.c
    ${MLN_TS_NATIVE}/runtime.c
    ${MLN_TS_NATIVE}/realm.c)
set_target_properties(
  maplibre_native_ffi_react
  PROPERTIES
    C_STANDARD
    11
    C_STANDARD_REQUIRED
    ON
    C_EXTENSIONS
    OFF
    CXX_STANDARD
    20
    CXX_STANDARD_REQUIRED
    ON
    C_VISIBILITY_PRESET
    hidden
    CXX_VISIBILITY_PRESET
    hidden)
target_include_directories(
  maplibre_native_ffi_react
  PRIVATE
    ${MLN_RN_NATIVE} ${MLN_TS_NATIVE} ${MLN_TS_NATIVE}/include
    ${MLN_TS_NATIVE}/node-api-headers)
target_compile_definitions(maplibre_native_ffi_react PRIVATE NAPI_VERSION=8)
target_compile_options(
  maplibre_native_ffi_react
  PRIVATE
    -include ${MLN_RN_NATIVE}/symbols.h -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(
  maplibre_native_ffi_react
  PRIVATE
    ReactAndroid::jsi ReactAndroid::reactnative fbjni::fbjni
    maplibre-native-c::maplibre-native-c)
