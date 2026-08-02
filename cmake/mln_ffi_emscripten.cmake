# Toolchain-wide settings for browser builds.
#
# These are directory-scoped rather than per-target on purpose: pthreads change
# the ABI, so every translation unit and every archive linked into a module has
# to agree on them. That includes the Rust platform library, which rebuilds
# `std` with atomics for the same reason (see cmake/mln_ffi_rust.cmake).

if(NOT EMSCRIPTEN)
  return()
endif()

set(MLN_FFI_EMSCRIPTEN_PTHREAD_POOL_SIZE "16"
    CACHE STRING "Emscripten pre-spawned pthread pool size")
set(MLN_FFI_EMSCRIPTEN_INITIAL_MEMORY "512MB"
    CACHE STRING "Initial WASM linear memory")
set(MLN_FFI_EMSCRIPTEN_STACK_SIZE "1MB"
    CACHE STRING "WASM stack size for native rendering code")

# MapLibre runs its tile work on threads, and a browser host drives the map from
# whichever pthread owns the runtime, so pthreads are not optional here. They
# require the page to be cross-origin isolated (COOP/COEP), which is a
# deployment constraint for anything embedding a browser build.
add_compile_options(-pthread)
add_link_options(
  -pthread "-sPTHREAD_POOL_SIZE=${MLN_FFI_EMSCRIPTEN_PTHREAD_POOL_SIZE}"
  "-sINITIAL_MEMORY=${MLN_FFI_EMSCRIPTEN_INITIAL_MEMORY}"
  "-sSTACK_SIZE=${MLN_FFI_EMSCRIPTEN_STACK_SIZE}")

# MapLibre leans on exceptions, so the model is load-bearing rather than
# incidental, and it is not the same on both backends.
#
# Native Wasm exceptions cost far less than emulating them in JavaScript, so
# WebGL uses them. WebGPU cannot: the emdawnwebgpu port turns on Asyncify to
# implement emwgpuWaitAny, and wasm-opt aborts when asked to run the Asyncify
# pass over a module using native exception handling. That only shows up at -O2
# and above, where wasm-opt runs at all, so an unoptimised link is not evidence
# either way.
#
# TODO(browser-webgpu): JSPI would let WebGPU keep native exceptions, and
# emdawnwebgpu already accepts it -- its own validation asks for "Asyncify or
# JSPI". What blocks it is upstream: MapLibre Native pins -sASYNCIFY=1 as an
# interface link option in vendor/dawn.cmake, and an interface option lands
# after
# ours, so a -sJSPI here is overridden. Adopting JSPI needs that flag to become
# configurable upstream first.
if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
  add_compile_options(-fexceptions)
  add_link_options(-fexceptions)
else()
  add_compile_options(-fwasm-exceptions)
  add_link_options(-fwasm-exceptions)
endif()
