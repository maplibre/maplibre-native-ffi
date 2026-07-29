function(mln_configure_render_dependencies target)
  mln_add_license(${target} "${MLN_SOURCE_DIR}/vendor/Vulkan-Headers/LICENSE.md"
                  "vulkan-headers.md")
  mln_add_license(
    ${target} "${MLN_SOURCE_DIR}/vendor/VulkanMemoryAllocator/LICENSE.txt"
    "vulkan-memory-allocator.txt")
  mln_add_license(${target} "${MLN_SOURCE_DIR}/vendor/glslang/LICENSE.txt"
                  "glslang.txt")

  get_target_property(MLN_FFI_VULKAN_INCLUDE_DIRS mbgl-vendor-vulkan-headers
                      INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_INCLUDE_DIRS "${MLN_FFI_VULKAN_INCLUDE_DIRS}"
      MLN_FFI_STATIC_ARCHIVES
      "glslang;SPIRV;glslang-default-resource-limits;OSDependent;MachineIndependent;GenericCodeGen;SPIRV-Tools;SPIRV-Tools-opt")

  # The loader belongs to the test harness, which drives Vulkan directly the way
  # a host does. The library resolves it at runtime, so build-host loader paths
  # stay out of the shipped binary, and a build without the harness needs no
  # loader for the target architecture at all. That is what lets a build for one
  # architecture run on another.
  if(NOT BUILD_TESTING)
    return()
  endif()

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
  add_library(mln_ffi_vulkan_loader UNKNOWN IMPORTED GLOBAL)
  set_target_properties(
    mln_ffi_vulkan_loader
    PROPERTIES IMPORTED_LOCATION "${MLN_FFI_VULKAN_LOADER_LIBRARY}")
  set_property(
    TARGET ${target}
    PROPERTY MLN_FFI_TEST_LINK_LIBRARIES mln_ffi_vulkan_loader)

  get_filename_component(
    MLN_FFI_VULKAN_LOADER_DIR "${MLN_FFI_VULKAN_LOADER_LIBRARY}"
    DIRECTORY)
  set_property(
    TARGET ${target}
    PROPERTY MLN_FFI_RUNTIME_DIRS "${MLN_FFI_VULKAN_LOADER_DIR}")

  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MLN_FFI_VULKAN_ICD_FILE "$ENV{VK_DRIVER_FILES}")
    if(NOT EXISTS "${MLN_FFI_VULKAN_ICD_FILE}")
      find_file(
        MLN_FFI_VULKAN_ICD_FILE
        NAMES MoltenVK_icd.json
        HINTS
          "$ENV{VULKAN_SDK}" PATH_SUFFIXES etc/vulkan/icd.d share/vulkan/icd.d
        REQUIRED)
    endif()
    set_property(
      TARGET ${target}
      PROPERTY MLN_FFI_VULKAN_ICD_FILE "${MLN_FFI_VULKAN_ICD_FILE}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # The SDK keeps the loader under runtime/<architecture>. Requiring it here
    # is what keeps the runtime directory honest: tests bind vulkan-1.dll at
    # load time, so accepting a miss leaves this property on the import library
    # directory and every test dies in the loader before reaching main.
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
    get_filename_component(
      MLN_FFI_VULKAN_RUNTIME_DIR "${MLN_FFI_VULKAN_RUNTIME}"
      DIRECTORY)
    set_property(
      TARGET ${target}
      PROPERTY MLN_FFI_RUNTIME_DIRS "${MLN_FFI_VULKAN_RUNTIME_DIR}")
  endif()
endfunction()

function(mln_configure_renderer target)
  set(MLN_FFI_VENDOR_VULKAN_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/vulkan/headless_backend.cpp)
  set(MLN_FFI_VULKAN_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_dispatch.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_backend.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_surface_session.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_VULKAN_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_VULKAN_SOURCES})

  target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
endfunction()
