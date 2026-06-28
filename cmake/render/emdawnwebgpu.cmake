function(mln_configure_emdawnwebgpu target)
  set(_emdawn_flags "--use-port=emdawnwebgpu")
  get_target_property(_target_type ${target} TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_scope INTERFACE)
  else()
    set(_scope PRIVATE)
  endif()

  target_compile_options(${target} ${_scope} "${_emdawn_flags}" "-fexceptions"
                         "-sUSE_ZLIB=1")
  target_link_options(${target} ${_scope} "${_emdawn_flags}" "-sUSE_ZLIB=1"
                      "-sFETCH=1" "-sENVIRONMENT=web,worker")
endfunction()

function(mln_prepare_emdawnwebgpu_vendor)
  # Vendor target is created by maplibre-native vendor/webgpu/emdawn.cmake.
endfunction()
