# Toolchain-wide settings for browser builds.
#
# These are directory-scoped rather than per-target on purpose: pthreads change
# the ABI, so every translation unit and every archive linked into a module has
# to agree on them. That includes the Rust platform library, which rebuilds
# `std` with atomics for the same reason (see cmake/mln_rust.cmake).

if(NOT EMSCRIPTEN)
  return()
endif()

set(MLN_EMSCRIPTEN_PTHREAD_POOL_SIZE "16"
    CACHE STRING "Emscripten pre-spawned pthread pool size")
set(MLN_EMSCRIPTEN_INITIAL_MEMORY "512MB"
    CACHE STRING "Initial WASM linear memory")
set(MLN_EMSCRIPTEN_STACK_SIZE "1MB"
    CACHE STRING "WASM stack size for native rendering code")

# MapLibre runs its tile work on threads, and a browser host drives the map from
# whichever pthread owns the runtime, so pthreads are not optional here. They
# require the page to be cross-origin isolated (COOP/COEP), which is a
# deployment constraint for anything embedding a browser build.
add_compile_options(-pthread)
add_link_options(
  -pthread "-sPTHREAD_POOL_SIZE=${MLN_EMSCRIPTEN_PTHREAD_POOL_SIZE}"
  "-sINITIAL_MEMORY=${MLN_EMSCRIPTEN_INITIAL_MEMORY}"
  "-sSTACK_SIZE=${MLN_EMSCRIPTEN_STACK_SIZE}")

# MapLibre leans on exceptions, so the model is load-bearing rather than
# incidental. Native Wasm exceptions cost far less than emulating them in
# JavaScript, and every browser that ships WebGL2 supports them.
#
# TODO(browser-webgpu): emdawnwebgpu waits on promises through Asyncify, which
# cannot mix with native Wasm exceptions. Adding the WebGPU backend means either
# moving it to JSPI or giving that backend -fexceptions.
add_compile_options(-fwasm-exceptions)
add_link_options(-fwasm-exceptions)
