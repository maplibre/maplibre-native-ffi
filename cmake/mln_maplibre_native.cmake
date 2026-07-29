function(mln_add_maplibre_native)
  set(MLN_SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/maplibre-native")

  if(MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # MapLibre otherwise selects the GCC spelling, -fno-rtti, based on Clang's
    # compiler ID. clang-cl uses the MSVC spelling added below instead.
    set(MLN_WITH_RTTI ON)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    # OHOS SDK 6.x exposes some libc++ C++20 facilities behind this clang flag.
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fexperimental-library>)
  endif()

  if(WIN32)
    add_compile_definitions(NOMINMAX GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
                            _USE_MATH_DEFINES)
  endif()

  add_subdirectory("${MLN_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/maplibre-native")

  if(MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
      mbgl-compiler-options
      INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/GR->)

    # The vendored tile-spec library's Unix -Wall flag means -Weverything to
    # clang-cl. Neutralize it to match the dependency's MSVC warning behavior.
    foreach(MLN_FFI_MLT_TARGET mlt-cpp mlt-cpp-encoder fastpfor-lib fsst-lib)
      if(TARGET ${MLN_FFI_MLT_TARGET})
        target_compile_options(${MLN_FFI_MLT_TARGET} PRIVATE -Wno-everything)
      endif()
    endforeach()
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    target_include_directories(
      mbgl-core
      BEFORE
      PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/ohos/compat)

    foreach(MLN_FFI_MLT_TARGET mlt-cpp mlt-cpp-encoder)
      if(TARGET ${MLN_FFI_MLT_TARGET})
        target_include_directories(
          ${MLN_FFI_MLT_TARGET}
          BEFORE
          PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/ohos/compat)
      endif()
    endforeach()
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(mbgl-core PRIVATE mbgl-vendor-filesystem)
  endif()

  include("${MLN_SOURCE_DIR}/vendor/nunicode.cmake")
  include("${MLN_SOURCE_DIR}/vendor/sqlite.cmake")

  set(MLN_SOURCE_DIR "${MLN_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
