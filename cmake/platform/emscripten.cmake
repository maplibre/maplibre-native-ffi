function(mln_ffi_configure_platform_dependencies target)
  # Consumers link the final module, so they need these options.
  target_compile_options(${target} INTERFACE -pthread)
  target_link_options(
    ${target}
    INTERFACE
      -pthread
      "-sPTHREAD_POOL_SIZE=${MLN_FFI_EMSCRIPTEN_PTHREAD_POOL_SIZE}"
      "-sINITIAL_MEMORY=${MLN_FFI_EMSCRIPTEN_INITIAL_MEMORY}"
      "-sALLOW_MEMORY_GROWTH=1"
      "-sMAXIMUM_MEMORY=${MLN_FFI_EMSCRIPTEN_MAXIMUM_MEMORY}"
      -Wno-pthreads-mem-growth
      "-sSTACK_SIZE=${MLN_FFI_EMSCRIPTEN_STACK_SIZE}")
  # TODO: Use SIDE_MODULE when pthread dynamic linking is stable.
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_SHARED_SUPPORTED FALSE MLN_FFI_ARCHIVE_FORMAT wasm
      MLN_FFI_TEST_SUPPORTED TRUE)
endfunction()
