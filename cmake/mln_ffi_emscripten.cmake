# Toolchain-wide settings for browser builds.
#
# These are directory-scoped rather than per-target on purpose: pthreads change
# the ABI, so every translation unit and every archive linked into a module has
# to agree on them. That includes the Rust platform library, which rebuilds
# `std` with atomics for the same reason (see cmake/mln_ffi_rust.cmake).

if(NOT EMSCRIPTEN)
  return()
endif()

# Emscripten pre-spawns this many Web Workers while the page loads. The pool is
# not a cap. Once it is empty, pthread_create() asks the browser main thread for
# a new Worker, and the thread starts when the main thread next yields. A call
# that holds the main thread until such a thread starts never returns, so
# mln_runtime_create() returns MLN_STATUS_WRONG_THREAD there, and a render
# session destroyed there detaches its core worker instead of joining it. Other
# calls wait only for work on threads that earlier calls started, such as
# mln_render_session_abandon() waiting for the tile work of the map's worker
# pool; a main thread that has yielded since those calls finds them running.
# The default covers one host thread that drives one runtime with one map, one
# GeoJSON source, and one render session, so such a page starts every thread
# without waiting for a Worker. A host that runs more raises the size.
#
# The thread that calls into the library, such as main() under
# -sPROXY_TO_PTHREAD.
set(_mln_pool_host 1)
# MapLibre's background ThreadPool, a ParallelScheduler(3) of four threads that
# every map shares.
set(_mln_pool_maplibre_workers 4)
# MapLibre's sequenced schedulers: one for logging and one for the first
# GeoJSON source. Further GeoJSON sources take up to eight more.
set(_mln_pool_maplibre_sequenced 2)
# The process-wide map teardown, session teardown, and runtime disposal lanes.
set(_mln_pool_lanes 3)
# The browser HTTP transport thread, which issues every fetch.
set(_mln_pool_transport 1)
# Each runtime's executor, plus the util::Thread of each file source its first
# map creates: the resource loader, asset, database, local file, network,
# MBTiles, and PMTiles sources.
set(_mln_pool_per_runtime 8)
# Each render session's core worker.
set(_mln_pool_per_session 1)
math(EXPR _mln_pool_default
     "${_mln_pool_host} + ${_mln_pool_maplibre_workers} + ${_mln_pool_maplibre_sequenced} + ${_mln_pool_lanes} + ${_mln_pool_transport} + ${_mln_pool_per_runtime} + ${_mln_pool_per_session}")
set(MLN_FFI_EMSCRIPTEN_PTHREAD_POOL_SIZE "${_mln_pool_default}"
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
