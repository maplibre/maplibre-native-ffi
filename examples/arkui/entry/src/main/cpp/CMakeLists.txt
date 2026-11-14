# The example's own native module. scripts/build-hap.sh configures it with the
# OpenHarmony SDK's toolchain file, OHOS_ARCH, and the facade's API floor as
# OHOS_COMPATIBLE_SDK_VERSION, and packs the library into the HAP.
cmake_minimum_required(VERSION 3.28)
project(maplibre_arkui_example LANGUAGES C)

find_library(ACE_NAPI NAMES ace_napi.z REQUIRED)
find_library(NATIVE_WINDOW NAMES native_window REQUIRED)

add_library(ohos_window SHARED ohos_window.c)
set_target_properties(
  ohos_window
  PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED YES C_VISIBILITY_PRESET hidden)
# The SDK toolchain passes --gcc-toolchain to every compile, and clang ignores
# it for C. clang checks the API floor with the availability warning.
target_compile_options(
  ohos_window
  PRIVATE
    -Wall -Wextra -Wno-unused-command-line-argument
    -Werror=unguarded-availability)
target_link_libraries(ohos_window PRIVATE "${ACE_NAPI}" "${NATIVE_WINDOW}")
