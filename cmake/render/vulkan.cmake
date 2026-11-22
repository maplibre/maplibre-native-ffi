# The library loads the Vulkan loader at run time, so it neither compiles
# against the Vulkan headers nor links a loader. The test graphics fixtures,
# which drive Vulkan directly the way a host does, try the loader found here
# before the platform's default search.
function(mln_ffi_configure_vulkan_dependencies target)
  # The loader's path is a build-host path, so only a native build with the
  # test graphics fixtures looks for one. A cross build, whose tests run
  # elsewhere, needs no loader for the target architecture at all.
  if(NOT BUILD_TESTING OR CMAKE_CROSSCOMPILING)
    return()
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # The SDK keeps the loader DLL under runtime/<architecture>.
    set(MLN_FFI_VULKAN_RUNTIME_SUFFIXES runtime/x64 runtime/x86 Bin bin)
    if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "arm64"
       OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$")
      set(MLN_FFI_VULKAN_RUNTIME_SUFFIXES runtime/arm64 runtime/ARM64 Bin bin)
    endif()
    find_file(
      MLN_FFI_VULKAN_RUNTIME
      NAMES vulkan-1.dll
      HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES ${MLN_FFI_VULKAN_RUNTIME_SUFFIXES}
      REQUIRED)
  else()
    set(MLN_FFI_VULKAN_LIBRARY_SUFFIXES Lib Lib32 Lib/arm64)
    if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "arm64"
       OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$")
      set(MLN_FFI_VULKAN_LIBRARY_SUFFIXES Lib/arm64 Lib Lib32)
    endif()
    find_library(
      MLN_FFI_VULKAN_LOADER_LIBRARY
      NAMES vulkan vulkan-1 vulkan.1
      HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES ${MLN_FFI_VULKAN_LIBRARY_SUFFIXES}
      REQUIRED)
  endif()
endfunction()
