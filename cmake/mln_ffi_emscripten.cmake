# Toolchain-wide settings for browser builds.
#
# These are directory-scoped rather than per-target on purpose: pthreads change
# the ABI, so every translation unit and every archive linked into a module has
# to agree on them. That includes the library, whose Cargo build rebuilds `std`
# with atomics for the same reason (see cmake/mln_ffi_mln_rs.cmake).

if(NOT EMSCRIPTEN)
  return()
endif()

# Emscripten pre-spawns this many Web Workers while the page loads. The pool is
# not a cap. Once it is empty, pthread_create() asks the browser main thread for
# a new Worker, and the thread starts when the main thread next yields. A call
# that holds the main thread until such a thread starts never returns. The
# default comes from the threads that the C++ implementation started for one
# host thread that drives one runtime with one map and one render session; it
# has not been re-derived for mln-rs.
set(_mln_pool_default 20)
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

# The library runs tile work on threads, and every runtime owns a thread,
# so pthreads are not optional here. They require the page to be cross-origin
# isolated (COOP/COEP), which is a deployment constraint for anything embedding
# a browser build.
add_compile_options(-pthread)

add_compile_options(-fwasm-exceptions)
