# The render backend of a preset selects which of the library's backends the
# test harness drives and which one the artifact descriptor names. The library
# itself carries every backend that mln-rs builds for the target.
function(mln_ffi_select_render_backend)
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

  mln_ffi_configure_render_dependencies(mln_ffi_render_dependencies)
  set_property(
    TARGET mln_ffi_render_dependencies
    PROPERTY MLN_FFI_RENDER_BACKEND "${MLN_FFI_RENDER_BACKEND}")
endfunction()
