# Private dependency linking for maplibre-native-c.
#
# Policy:
# - Platforms that ship zlib/libuv in the system/SDK link those directly.
# - Linux and Windows embed static archives into maplibre-native-c so consumers
#   do not ship separate libuv or zlib shared libraries.

function(mln_require_dependency_dir out_var)
  set(${out_var} "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}")
  if("${${out_var}}" STREQUAL "")
    message(
      FATAL_ERROR
        "MLN_FFI_DEPENDENCY_LIBRARY_DIR must be set for bundled private dependencies")
  endif()
  set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

function(mln_hide_bundled_static_symbols target)
  if(MLN_FFI_ARTIFACT_SHAPE STREQUAL "shared-private"
     AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(${target} PRIVATE "LINKER:--exclude-libs,ALL")
  endif()
endfunction()

function(mln_link_bundled_zlib target)
  mln_require_dependency_dir(dependency_lib_dir)

  if(WIN32)
    set(zlib_static_library "${dependency_lib_dir}/zlibstatic.lib")
  else()
    set(zlib_static_library "${dependency_lib_dir}/libz.a")
  endif()

  if(NOT EXISTS "${zlib_static_library}")
    message(
      FATAL_ERROR "Bundled static zlib not found at ${zlib_static_library}")
  endif()

  find_package(ZLIB REQUIRED)
  target_include_directories(${target} SYSTEM PRIVATE ${ZLIB_INCLUDE_DIR})

  if(NOT TARGET mln_bundled_zlib)
    add_library(mln_bundled_zlib STATIC IMPORTED GLOBAL)
  endif()

  set_target_properties(
    mln_bundled_zlib
    PROPERTIES IMPORTED_LOCATION "${zlib_static_library}")

  target_link_libraries(${target} PRIVATE mln_bundled_zlib)
  mln_hide_bundled_static_symbols(${target})
endfunction()

function(mln_configure_zlib_linking target)
  if(
    APPLE
    OR
    CMAKE_SYSTEM_NAME
    STREQUAL
    "Android"
    OR
    CMAKE_SYSTEM_NAME
    STREQUAL
    "OHOS")
    target_link_libraries(${target} PRIVATE z)
    return()
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32)
    mln_link_bundled_zlib(${target})
    return()
  endif()

  message(
    FATAL_ERROR "Unsupported platform for zlib linking: ${CMAKE_SYSTEM_NAME}")
endfunction()

function(mln_link_bundled_libuv target)
  mln_require_dependency_dir(dependency_lib_dir)

  find_path(
    LIBUV_INCLUDE_DIR
    NAMES uv.h
    HINTS
      "$ENV{MLN_FFI_DEPENDENCY_INCLUDE_DIR}" "${dependency_lib_dir}/../include"
    REQUIRED)

  if(WIN32)
    find_package(libuv REQUIRED)
    target_include_directories(${target} SYSTEM PRIVATE ${LIBUV_INCLUDE_DIR})
    target_link_libraries(${target} PRIVATE libuv::uv_a)
    return()
  endif()

  set(libuv_static_library "${dependency_lib_dir}/libuv.a")
  if(NOT EXISTS "${libuv_static_library}")
    message(
      FATAL_ERROR "Bundled static libuv not found at ${libuv_static_library}")
  endif()

  if(NOT TARGET mln_bundled_libuv)
    add_library(mln_bundled_libuv STATIC IMPORTED GLOBAL)
  endif()

  set_target_properties(
    mln_bundled_libuv
    PROPERTIES IMPORTED_LOCATION "${libuv_static_library}")

  target_include_directories(${target} SYSTEM PRIVATE ${LIBUV_INCLUDE_DIR})
  target_link_libraries(${target} PRIVATE mln_bundled_libuv dl)
  mln_hide_bundled_static_symbols(${target})
endfunction()

function(mln_link_bundled_png target)
  mln_require_dependency_dir(dependency_lib_dir)
  find_package(PNG REQUIRED)

  if(WIN32)
    set(png_static_candidates "${dependency_lib_dir}/libpng16_static.lib"
        "${dependency_lib_dir}/libpng_static.lib")
    set(png_static_library "")
    foreach(candidate IN LISTS png_static_candidates)
      if(EXISTS "${candidate}")
        set(png_static_library "${candidate}")
        break()
      endif()
    endforeach()
  else()
    set(png_static_library "${dependency_lib_dir}/libpng16.a")
  endif()

  if(NOT EXISTS "${png_static_library}")
    message(
      FATAL_ERROR "Bundled static libpng not found at ${png_static_library}")
  endif()

  if(NOT TARGET mln_bundled_png)
    add_library(mln_bundled_png STATIC IMPORTED GLOBAL)
  endif()

  set_target_properties(
    mln_bundled_png
    PROPERTIES IMPORTED_LOCATION "${png_static_library}")

  target_include_directories(${target} SYSTEM PRIVATE ${PNG_INCLUDE_DIRS})
  target_link_libraries(${target} PRIVATE mln_bundled_png)
  mln_hide_bundled_static_symbols(${target})
endfunction()
