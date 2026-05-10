function(mln_add_maplibre_native)
  set(MLN_SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/maplibre-native")

  if(WIN32)
    add_compile_definitions(NOMINMAX GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
                            _USE_MATH_DEFINES)
  endif()

  if(NOT EXISTS "${MLN_SOURCE_DIR}/CMakeLists.txt")
    message(
      FATAL_ERROR
        "MapLibre Native submodule is missing. Run `mise install` or `git submodule update --init --recursive --depth 1 third_party/maplibre-native`.")
  endif()

  add_subdirectory("${MLN_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/maplibre-native")

  include("${MLN_SOURCE_DIR}/vendor/nunicode.cmake")
  include("${MLN_SOURCE_DIR}/vendor/sqlite.cmake")

  set(MLN_SOURCE_DIR "${MLN_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
