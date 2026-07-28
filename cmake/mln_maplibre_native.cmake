function(mln_seed_mlt_compiler_flags)
  # maplibre-tile-spec probes a fixed list of warning flags one process at a
  # time. These spellings are deterministic for GCC and Unix Clang, so seed the
  # cache instead of paying dozens of try-compiles on every fresh target.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(unsupported -Wshorten-64-to-32 -wd4061 -wd4514 -wd4710 -wd4820)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang)$" AND NOT MSVC)
    set(unsupported -wd4061 -wd4514 -wd4710 -wd4820)
  else()
    return()
  endif()

  set(flags
      -Wall
      -Werror
      -Wextra
      -Wdeprecated-declarations
      -Winvalid-offsetof
      -Wno-block-capture-autoreleasing
      -Wno-bool-conversion
      -Wno-c++11-extensions
      -Wno-comma
      -Wno-constant-conversion
      -Wno-conversion
      -Wno-empty-body
      -Wno-enum-conversion
      -Wno-exit-time-destructors
      -Wno-float-conversion
      -Wno-four-char-constants
      -Wno-implicit-fallthrough
      -Wno-infinite-recursion
      -Wno-missing-braces
      -Wno-missing-field-initializers
      -Wno-move
      -Wno-newline-eof
      -Wno-non-literal-null-conversion
      -Wno-non-virtual-dtor
      -Wno-objc-literal-conversion
      -Wno-overloaded-virtual
      -Wno-range-loop-analysis
      -Wno-return-type
      -Wno-semicolon-before-method-body
      -Wno-shadow
      -Wno-sign-conversion
      -Wno-trigraphs
      -Wno-uninitialized
      -Wno-unknown-pragmas
      -Wno-unused-function
      -Wno-unused-label
      -Wno-unused-parameter
      -Wno-unused-variable
      -Wparentheses
      -Wshorten-64-to-32
      -Wswitch
      -Wunused-value
      -fstrict-aliasing
      -wd4061
      -wd4514
      -wd4710
      -wd4820)
  foreach(flag IN LISTS flags)
    string(TOUPPER "HAVE_CXX_FLAG_${flag}" variable)
    string(REPLACE "+" "X" variable "${variable}")
    string(REGEX REPLACE "[^A-Za-z_0-9]" "_" variable "${variable}")
    string(REGEX REPLACE "_+" "_" variable "${variable}")
    if(flag IN_LIST unsupported)
      set(${variable} OFF CACHE INTERNAL "")
    else()
      set(${variable} ON CACHE INTERNAL "")
    endif()
  endforeach()
endfunction()

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

  mln_seed_mlt_compiler_flags()
  add_subdirectory("${MLN_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/maplibre-native")

  if(MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
      mbgl-compiler-options
      INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/GR->)

    # The vendored tile-spec library's Unix -Wall flag means -Weverything to
    # clang-cl. Neutralize it to match the dependency's MSVC warning behavior.
    target_compile_options(mlt-cpp PRIVATE -Wno-everything)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    target_include_directories(
      mbgl-core
      BEFORE
      PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/ohos/compat)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(mbgl-core PRIVATE mbgl-vendor-filesystem)
  endif()

  include("${MLN_SOURCE_DIR}/vendor/nunicode.cmake")
  include("${MLN_SOURCE_DIR}/vendor/sqlite.cmake")

  set(MLN_SOURCE_DIR "${MLN_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
