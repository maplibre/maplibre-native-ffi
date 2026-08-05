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

# The heap above is fixed, so running out of it is a thing this module has to be
# able to say rather than a thing that cannot happen. Emscripten's default says
# it by aborting: `emscripten_resize_heap` calls `abortOnCannotGrowMemory` the
# first time an allocation needs a byte past the initial memory, which takes the
# whole module down and leaves a host holding handles it can no longer call and
# no error it could have caught. Every null check above a `malloc` in this
# repository is dead code under that default.
#
# So this build turns it off, and the two allocation paths then report instead.
# `malloc` returns null, which the C code and the bindings check; `operator new`
# throws `std::bad_alloc`, because the module links the exception-enabled
# libc++abi that `-fwasm-exceptions` selects, and `mln::c_api::status_boundary`
# catches it and returns MLN_STATUS_NATIVE_ERROR with its message. What is left
# unreportable is an allocation deep on a MapLibre worker thread, where there is
# no C API call to return to and an escaping `std::bad_alloc` terminates. That
# is what the default did to every allocation anyway.
#
# Growth is a separate, capacity-shaped decision that this one does not make.
# It moves the wall to MAXIMUM_MEMORY rather than removing it, so a browser
# build still has to report the failure either way, and it replaces every
# JavaScript view of the heap on each grow, so a binding holding one would have
# to re-read it per access. A host that needs more memory raises
# MLN_FFI_EMSCRIPTEN_INITIAL_MEMORY.
add_link_options(-sABORTING_MALLOC=0)

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
