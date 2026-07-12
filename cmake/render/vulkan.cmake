function(mln_configure_render_dependencies target)
  find_library(
    MLN_FFI_VULKAN_LOADER_LIBRARY
    NAMES vulkan vulkan-1 vulkan.1
    HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES Lib Lib32 Lib/arm64
    REQUIRED)
  add_library(mln_ffi_vulkan_loader UNKNOWN IMPORTED GLOBAL)
  set_target_properties(
    mln_ffi_vulkan_loader
    PROPERTIES IMPORTED_LOCATION "${MLN_FFI_VULKAN_LOADER_LIBRARY}")
  target_link_libraries(${target} INTERFACE mln_ffi_vulkan_loader)

  get_filename_component(
    MLN_FFI_VULKAN_LOADER_DIR "${MLN_FFI_VULKAN_LOADER_LIBRARY}"
    DIRECTORY)
  get_target_property(MLN_FFI_VULKAN_INCLUDE_DIRS mbgl-vendor-vulkan-headers
                      INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_INCLUDE_DIRS
      "${MLN_FFI_VULKAN_INCLUDE_DIRS}"
      MLN_FFI_LIBRARY_DIRS
      "${MLN_FFI_VULKAN_LOADER_DIR}"
      MLN_FFI_RUNTIME_DIRS
      "${MLN_FFI_VULKAN_LOADER_DIR}"
      MLN_FFI_STATIC_ARCHIVES
      "glslang;SPIRV;glslang-default-resource-limits;OSDependent;MachineIndependent;GenericCodeGen;SPIRV-Tools;SPIRV-Tools-opt")

  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_file(
      MLN_FFI_VULKAN_ICD_FILE
      NAMES MoltenVK_icd.json PATH_SUFFIXES share/vulkan/icd.d
      REQUIRED)
    set_property(
      TARGET ${target}
      PROPERTY MLN_FFI_VULKAN_ICD_FILE "${MLN_FFI_VULKAN_ICD_FILE}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_file(
      MLN_FFI_VULKAN_RUNTIME
      NAMES vulkan-1.dll
      HINTS "$ENV{VULKAN_SDK}" PATH_SUFFIXES Bin bin)
    if(MLN_FFI_VULKAN_RUNTIME)
      get_filename_component(
        MLN_FFI_VULKAN_RUNTIME_DIR "${MLN_FFI_VULKAN_RUNTIME}"
        DIRECTORY)
      set_property(
        TARGET ${target}
        PROPERTY MLN_FFI_RUNTIME_DIRS "${MLN_FFI_VULKAN_RUNTIME_DIR}")
    endif()
  endif()
endfunction()

function(mln_configure_renderer target)
  set(MLN_FFI_VENDOR_VULKAN_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/vulkan/headless_backend.cpp)
  set(MLN_FFI_VULKAN_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_backend.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_surface_session.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_VULKAN_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_VULKAN_SOURCES})

  target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
endfunction()
