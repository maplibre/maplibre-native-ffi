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
# -sPROXY_TO_PTHREAD, or the thread on which a binding that calls from the
# main thread creates each runtime, as the Kotlin/Wasm binding does.
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
math(
  EXPR _mln_pool_default
  "${_mln_pool_host} + ${_mln_pool_maplibre_workers} + ${_mln_pool_maplibre_sequenced} + ${_mln_pool_lanes} + ${_mln_pool_transport} + ${_mln_pool_per_runtime} + ${_mln_pool_per_session}"
)
set(MLN_FFI_EMSCRIPTEN_PTHREAD_POOL_SIZE "${_mln_pool_default}"
    CACHE STRING "Emscripten pre-spawned pthread pool size")
# The heap starts at the initial size and grows up to the maximum: a map on a
# large HiDPI canvas can hold several hundred megabytes of tiles and GPU
# uploads. Growth replaces the buffer behind JavaScript's views of the heap.
# The module checks its own views before each access, so JavaScript outside the
# module reads the heap only through the module, never through a view it keeps.
# A growable SharedArrayBuffer would remove those checks, but needs Chrome 144,
# Firefox 154, or Safari 26.2, so the link silences Emscripten's warning about
# their cost.
set(MLN_FFI_EMSCRIPTEN_INITIAL_MEMORY "512MB"
    CACHE STRING "Initial WASM linear memory")
set(MLN_FFI_EMSCRIPTEN_MAXIMUM_MEMORY "2GB"
    CACHE STRING "Maximum WASM linear memory")
set(MLN_FFI_EMSCRIPTEN_STACK_SIZE "1MB"
    CACHE STRING "WASM stack size for native rendering code")

# MapLibre runs tile work on threads, and every runtime owns a worker pthread,
# so pthreads are not optional here. They require the page to be cross-origin
# isolated (COOP/COEP), which is a deployment constraint for anything embedding
# a browser build.
add_compile_options(-pthread)

# WebGPU is asynchronous in a browser. Nothing in the library waits on it: a
# WebGPU offscreen session's core worker requests its device and reads back
# from callbacks that its event loop runs, as a WebGL one runs from its event
# loop, and a caller-driven session reports through the same callbacks. So
# nothing suspends, and the module needs neither JSPI nor Asyncify, which
# keeps the build in browsers without JSPI, such as Safari. MapLibre's own
# suspension option defaults to Asyncify, so the build clears it.
if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
  set(MLN_WEBGPU_EMDAWN_SUSPEND ""
      CACHE INTERNAL "No suspension: the library never waits on WebGPU")
endif()
add_compile_options(-fwasm-exceptions)
