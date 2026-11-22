# The graphics libraries of the backends that the library carries. Each backend
# has an interface target, mln_ffi_render_dependencies_<backend>, and
# mln_ffi_render_dependencies links every one of them. It is what the library's
# consumers link.
include(render/metal)
include(render/opengl)
include(render/vulkan)

function(mln_ffi_select_render_backend)
  add_library(mln_ffi_render_dependencies INTERFACE)
  add_library(MLN_FFI::RenderDependencies ALIAS mln_ffi_render_dependencies)

  foreach(backend IN LISTS MLN_FFI_RENDER_BACKENDS)
    set(target mln_ffi_render_dependencies_${backend})
    add_library(${target} INTERFACE)
    cmake_language(CALL mln_ffi_configure_${backend}_dependencies ${target})
    target_link_libraries(mln_ffi_render_dependencies INTERFACE ${target})
  endforeach()
  set_property(
    TARGET mln_ffi_render_dependencies
    PROPERTY MLN_FFI_RENDER_BACKENDS "${MLN_FFI_RENDER_BACKENDS}")
endfunction()
