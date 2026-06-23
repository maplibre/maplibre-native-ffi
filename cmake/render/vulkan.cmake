function(mln_configure_vulkan_backend target)
  set(_mln_vulkan_loader_names vulkan vulkan-1 vulkan.1)
  set(_mln_vulkan_loader_hints)
  if(DEFINED ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR} AND NOT
                                       "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}" STREQUAL "")
    list(APPEND _mln_vulkan_loader_hints "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}")
  endif()

  find_library(
    MLN_VULKAN_LOADER_LIBRARY
    NAMES ${_mln_vulkan_loader_names}
    HINTS ${_mln_vulkan_loader_hints}
    REQUIRED)

  set(MLN_FFI_VENDOR_VULKAN_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/vulkan/headless_backend.cpp)
  set(MLN_FFI_VULKAN_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_backend.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_surface_session.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_VULKAN_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_VULKAN_SOURCES})

  if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    target_compile_definitions(${target} PUBLIC VK_USE_PLATFORM_OHOS=1)
  endif()

  target_link_libraries(${target} PRIVATE ${MLN_VULKAN_LOADER_LIBRARY})
endfunction()
