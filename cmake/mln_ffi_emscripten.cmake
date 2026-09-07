# Toolchain-wide settings for browser builds.
#
# These are directory-scoped rather than per-target on purpose: pthreads change
# the ABI, so every translation unit and every archive linked into a module has
# to agree on them. That includes the Rust platform library, which rebuilds
# `std` with atomics for the same reason (see cmake/mln_ffi_rust.cmake).

if(NOT EMSCRIPTEN)
  return()
endif()

# The shared map teardown lane is reserved before map worker pools start.
set(MLN_FFI_EMSCRIPTEN_PTHREAD_POOL_SIZE "17"
    CACHE STRING "Emscripten pre-spawned pthread pool size")
set(MLN_FFI_EMSCRIPTEN_INITIAL_MEMORY "512MB"
    CACHE STRING "Initial WASM linear memory")
set(MLN_FFI_EMSCRIPTEN_STACK_SIZE "1MB"
    CACHE STRING "WASM stack size for native rendering code")

# MapLibre runs tile work on threads, and every runtime owns a worker pthread,
# so pthreads are not optional here. They require the page to be cross-origin
# isolated (COOP/COEP), which is a deployment constraint for anything embedding
# a browser build.
add_compile_options(-pthread)

# WebGPU is asynchronous in a browser and MapLibre calls it synchronously, so
# the emdawnwebgpu port implements emwgpuWaitAny by suspending the calling
# thread. Emscripten offers two mechanisms for that, and the choice reaches
# further than it looks: it decides the exception model and whether a thread
# that waits on WebGPU can be joined.
#
# JSPI suspends in the VM. Nothing rewrites the module, so native Wasm
# exceptions stay available, and emscripten's pthread glue awaits the entry
# point, which is what marks a suspended thread exited so pthread_join returns.
#
# Asyncify instead rewrites the module through wasm-opt, which costs size and
# speed, aborts outright over a module built with native exception handling
# (only at -O2 and above, where wasm-opt runs at all, so an unoptimised link is
# not evidence either way), and cannot carry a pthread entry point across a
# suspension: invokeEntryPoint decides whether to exit the moment the entry
# yields, which a suspension does immediately, and skips the exit because
# Asyncify holds a runtime keepalive right then. Nothing ever reports the
# thread as exited.
#
# So the browser WebGPU build selects JSPI, which restricts it to Chrome 137
# and Firefox 139 or newer -- Safari has not shipped JSPI. WebGL suspends
# nothing and is unaffected.
if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
  set(MLN_WEBGPU_EMDAWN_SUSPEND "-sJSPI"
      CACHE
        STRING
        "Emscripten suspension link option for emdawnwebgpu (-sASYNCIFY=1 or -sJSPI)")
endif()
add_compile_options(-fwasm-exceptions)
