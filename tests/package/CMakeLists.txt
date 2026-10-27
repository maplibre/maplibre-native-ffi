cmake_minimum_required(VERSION 3.28)
project(native_package_consumer LANGUAGES C)

find_package(maplibre-native-c CONFIG REQUIRED)

add_executable(shared_consumer main.c)
target_link_libraries(
  shared_consumer
  PRIVATE maplibre-native-c::maplibre-native-c)

if(EXISTS "${MLN_PACKAGE_PREFIX}/lib/libmaplibre-native-c.a"
   OR EXISTS "${MLN_PACKAGE_PREFIX}/lib/maplibre-native-c-static.lib")
  add_executable(static_consumer main.c)
  target_link_libraries(
    static_consumer
    PRIVATE maplibre-native-c::maplibre-native-c-static)
endif()
