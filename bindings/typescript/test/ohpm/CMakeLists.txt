cmake_minimum_required(VERSION 3.28)
project(ohpm_consumer LANGUAGES C)

add_library(consumer SHARED consumer.c)
target_include_directories(consumer PRIVATE "${MLN_FFI_OHPM_PACKAGE}/include")
target_link_libraries(
  consumer
  PRIVATE "${MLN_FFI_OHPM_PACKAGE}/libs/${OHOS_ARCH}/libmaplibre-native-c.so")
target_link_options(consumer PRIVATE "LINKER:--no-undefined")
