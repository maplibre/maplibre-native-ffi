function(mln_add_maplibre_native)
  set(MLN_SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/maplibre-native")

  if(WIN32)
    add_compile_definitions(NOMINMAX GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
                            _USE_MATH_DEFINES)
  endif()

  add_subdirectory("${MLN_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/maplibre-native")

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(mbgl-core PRIVATE mbgl-vendor-filesystem)
  endif()

  include("${MLN_SOURCE_DIR}/vendor/nunicode.cmake")
  include("${MLN_SOURCE_DIR}/vendor/sqlite.cmake")

  set(MLN_SOURCE_DIR "${MLN_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
