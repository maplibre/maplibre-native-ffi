cmake_minimum_required(VERSION 3.22.1)

project(maplibre_android_map_example LANGUAGES CXX)

add_library(android_map_vulkan SHARED vulkan_context.cpp)

target_compile_features(android_map_vulkan PRIVATE cxx_std_20)
target_compile_definitions(
  android_map_vulkan
  PRIVATE VK_USE_PLATFORM_ANDROID_KHR=1)

target_link_libraries(android_map_vulkan PRIVATE android log vulkan)
