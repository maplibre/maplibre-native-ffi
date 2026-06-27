# Global Emscripten toolchain settings for browser/WASM builds.
#
# Pthreads require atomics/bulk-memory on every translation unit, so -pthread is
# applied at the top level rather than per-target when enabled. Growing the heap
# is not compatible with pthreads; use a generous fixed initial memory instead.

if(NOT EMSCRIPTEN)
  return()
endif()

set(MLN_EMSCRIPTEN_PTHREAD_POOL_SIZE "16"
    CACHE STRING "Emscripten pre-spawned pthread pool size")

set(MLN_EMSCRIPTEN_INITIAL_MEMORY "256MB"
    CACHE
      STRING "Initial WASM linear memory (pthread builds cannot grow the heap)")

set(MLN_EMSCRIPTEN_STACK_SIZE "1MB"
    CACHE STRING "WASM stack size for native C++ rendering code")

set(MLN_EMSCRIPTEN_USE_PTHREADS ON
    CACHE BOOL "Build Emscripten targets with pthread support")

add_compile_options(-fwasm-exceptions)
add_link_options(
  -fwasm-exceptions "-sINITIAL_MEMORY=${MLN_EMSCRIPTEN_INITIAL_MEMORY}"
  "-sSTACK_SIZE=${MLN_EMSCRIPTEN_STACK_SIZE}")

if(MLN_EMSCRIPTEN_USE_PTHREADS)
  set(_mln_emscripten_pthread_link_flags -pthread -sUSE_PTHREADS=1
      "-sPTHREAD_POOL_SIZE=${MLN_EMSCRIPTEN_PTHREAD_POOL_SIZE}")

  add_compile_options(-pthread)
  add_link_options(${_mln_emscripten_pthread_link_flags})
else()
  add_compile_definitions(MLN_EMSCRIPTEN_SINGLE_THREADED=1)
  add_link_options(-sALLOW_MEMORY_GROWTH=1)
endif()
