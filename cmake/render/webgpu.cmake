function(mln_ffi_configure_render_dependencies target)
  # emdawnwebgpu supplies webgpu.h and its JavaScript bindings against the
  # browser's own navigator.gpu, for the hosts that hand the library a device.
  target_compile_options(${target} INTERFACE "--use-port=emdawnwebgpu")
  target_link_options(${target} INTERFACE "--use-port=emdawnwebgpu")
endfunction()
