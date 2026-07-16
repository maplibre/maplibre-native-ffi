function(mln_select_render_backend)
  add_library(mln_ffi_render_dependencies INTERFACE)
  add_library(MLN_FFI::RenderDependencies ALIAS mln_ffi_render_dependencies)

  if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
    include(render/metal)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    include(render/opengl)
  elseif(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    include(render/vulkan)
  else()
    message(FATAL_ERROR "Unsupported render backend: ${MLN_FFI_RENDER_BACKEND}")
  endif()

  mln_configure_render_dependencies(mln_ffi_render_dependencies)
  set_property(
    TARGET mln_ffi_render_dependencies
    PROPERTY MLN_FFI_RENDER_BACKEND "${MLN_FFI_RENDER_BACKEND}")
endfunction()

function(mln_configure_render_backend target)
  mln_configure_renderer(${target})
endfunction()
