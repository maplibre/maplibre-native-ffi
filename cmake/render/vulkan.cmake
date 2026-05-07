function(mln_configure_vulkan_backend target)
  find_library(MLN_VULKAN_LOADER_LIBRARY NAMES vulkan REQUIRED)

  set(MLN_FFI_VENDOR_VULKAN_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/vulkan/headless_backend.cpp)
  set(MLN_FFI_VULKAN_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_backend.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_surface_session.cpp)

  target_sources(
    ${target}
    PRIVATE ${MLN_FFI_VENDOR_VULKAN_SOURCES} ${MLN_FFI_VULKAN_SOURCES})
  foreach(source IN LISTS MLN_FFI_VENDOR_VULKAN_SOURCES)
    mln_configure_vendor_source(${source})
  endforeach()
  foreach(source IN LISTS MLN_FFI_VULKAN_SOURCES)
    mln_configure_project_source(${source})
  endforeach()

  target_link_libraries(${target} PRIVATE ${MLN_VULKAN_LOADER_LIBRARY})
endfunction()
