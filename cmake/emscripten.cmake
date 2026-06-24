# Global Emscripten toolchain settings for browser/WASM builds.
#
# Pthreads require atomics/bulk-memory on every translation unit, so -pthread is
# applied at the top level rather than per-target. Growing the heap is not
# compatible with pthreads; use a generous fixed initial memory instead.

if(NOT EMSCRIPTEN)
  return()
endif()

set(MLN_EMSCRIPTEN_PTHREAD_POOL_SIZE
    "4"
    CACHE STRING "Emscripten pre-spawned pthread pool size")

set(MLN_EMSCRIPTEN_INITIAL_MEMORY
    "256MB"
    CACHE STRING "Initial WASM linear memory (pthread builds cannot grow the heap)")

set(_mln_emscripten_pthread_link_flags
    -pthread
    -sUSE_PTHREADS=1
    "-sPTHREAD_POOL_SIZE=${MLN_EMSCRIPTEN_PTHREAD_POOL_SIZE}"
    "-sINITIAL_MEMORY=${MLN_EMSCRIPTEN_INITIAL_MEMORY}")

add_compile_options(-pthread)
add_link_options(${_mln_emscripten_pthread_link_flags})
