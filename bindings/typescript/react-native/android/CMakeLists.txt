# The module's library: the binding's Hermes adapter linked against React
# Native's JSI, the shared C++, and the Android glue.
cmake_minimum_required(VERSION 3.22)
project(maplibre_native_ffi_react_native LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gradle reaches this file through the app's node_modules link.
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." PACKAGE_ROOT)
file(REAL_PATH "${PACKAGE_ROOT}/../../.." MLN_FFI_ROOT)

# The C library for this ABI, from `mise run //:build android-<arch>-egl`.
set(abis_armeabi-v7a arm)
set(abis_arm64-v8a arm64)
set(abis_x86 x86)
set(abis_x86_64 x64)
set(preset "android-${abis_${ANDROID_ABI}}-egl")
if(NOT MLN_FFI_NATIVE_PREFIX)
  set(MLN_FFI_NATIVE_PREFIX "${MLN_FFI_ROOT}/build/${preset}/install")
endif()
if(NOT EXISTS "${MLN_FFI_NATIVE_PREFIX}/include/maplibre_native_c.h")
  message(
    FATAL_ERROR
      "The C library for ${ANDROID_ABI} isn't built. Build it with:\n"
      "  mise -E android run //:build ${preset}")
endif()

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

# The binding's adapter takes React Native's JSI as a target named jsi.
add_library(jsi INTERFACE)
target_link_libraries(jsi INTERFACE ReactAndroid::jsi)
get_target_property(MLN_HERMES_JSI_DIR ReactAndroid::jsi
                    INTERFACE_INCLUDE_DIRECTORIES)
set(MLN_TS_NATIVE "${PACKAGE_ROOT}/node_modules/@maplibre/native-ffi/native")
add_subdirectory("${MLN_TS_NATIVE}/hermes" binding)
# The npm package carries the C headers in native/include, which the
# workspace's sources leave to the repository.
target_include_directories(
  maplibre_native_ffi_hermes
  PRIVATE "${MLN_FFI_ROOT}/include")

add_library(
  maplibre_native_ffi_react_native
  SHARED
    "${PACKAGE_ROOT}/cpp/Host.cpp" "${PACKAGE_ROOT}/cpp/Surfaces.cpp"
    src/main/jni/OnLoad.cpp)
target_include_directories(
  maplibre_native_ffi_react_native
  PRIVATE "${PACKAGE_ROOT}/cpp" "${MLN_FFI_NATIVE_PREFIX}/include")
target_link_libraries(
  maplibre_native_ffi_react_native
  PRIVATE
    maplibre_native_ffi_hermes ReactAndroid::jsi ReactAndroid::reactnative
    fbjni::fbjni android)
