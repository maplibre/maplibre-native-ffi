cmake_minimum_required(VERSION 3.22)
project(appmodules LANGUAGES C CXX)

include(${REACT_ANDROID_DIR}/cmake-utils/ReactNative-application.cmake)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)
set(MLN_NATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../node_modules/@maplibre/native-ffi/native")
add_library(
  mln_lifecycle_probe
  SHARED
    jni/adapter.cpp
    ../native/Probe.cpp
    ../native/shim_test.c
    ../native/test_graphics.c
    ../support/native_probe.c
    ../graphics/graphics.c
    ../native/shim.cpp)
target_include_directories(
  mln_lifecycle_probe
  PRIVATE
    ../native ../support ../graphics/include ../vulkan
    ${MLN_NATIVE}/react-native ${MLN_NATIVE}/node-api-headers)
target_compile_options(
  mln_lifecycle_probe
  PRIVATE -include ${MLN_NATIVE}/react-native/symbols.h)
set_target_properties(
  mln_lifecycle_probe
  PROPERTIES
    C_STANDARD
    11
    C_STANDARD_REQUIRED
    ON
    C_EXTENSIONS
    OFF
    CXX_VISIBILITY_PRESET
    hidden
    C_VISIBILITY_PRESET
    hidden)
target_compile_features(mln_lifecycle_probe PRIVATE cxx_std_20)
target_link_libraries(
  mln_lifecycle_probe
  PRIVATE
    ReactAndroid::jsi ReactAndroid::reactnative fbjni::fbjni log android dl)
