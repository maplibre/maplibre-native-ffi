# The graphics libraries of the backends that the library carries. Each backend
# has an interface target, mln_ffi_render_dependencies_<backend>, whose
# properties tell the test harness what it compiles and links against to drive
# that backend the way a host does:
#
#   MLN_FFI_INCLUDE_DIRS         the graphics API's headers
#   MLN_FFI_TEST_LINK_LIBRARIES  the loader that the harness links
#   MLN_FFI_RUNTIME_DIRS         where the tests find that loader
#
# mln_ffi_render_dependencies links every backend's target, and is what the
# library's consumers link.
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

# Reads a property of a backend's dependency target, empty where it is unset.
function(mln_ffi_render_dependency_property out_var backend property)
  get_target_property(value mln_ffi_render_dependencies_${backend} ${property})
  if("${value}" MATCHES "-NOTFOUND$")
    set(value "")
  endif()
  set(${out_var} "${value}" PARENT_SCOPE)
endfunction()
