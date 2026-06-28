if(TARGET mln-vendor-libuv)
  return()
endif()

if(TARGET uv_a)
  add_library(mln-vendor-libuv ALIAS uv_a)
  return()
endif()

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)

fetchcontent_declare(
  mln_libuv
  GIT_REPOSITORY https://github.com/libuv/libuv.git
  GIT_TAG v1.49.2
  GIT_SHALLOW TRUE)

fetchcontent_makeavailable(mln_libuv)

add_library(mln-vendor-libuv ALIAS uv_a)
