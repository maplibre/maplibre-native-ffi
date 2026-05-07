function(mln_configure_vulkan_backend target)
  find_library(MLN_VULKAN_LOADER_LIBRARY NAMES vulkan REQUIRED)

  target_sources(
    ${target}
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/vulkan/headless_backend.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_texture_backend.cpp
      ${PROJECT_SOURCE_DIR}/src/render/vulkan/vulkan_surface_session.cpp)

  target_link_libraries(${target} PRIVATE ${MLN_VULKAN_LOADER_LIBRARY})
endfunction()
