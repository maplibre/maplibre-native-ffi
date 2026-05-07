function(mln_fetch_or_add_maplibre_native)
  # MLN_SOURCE_DIR can be set externally to point at a local MapLibre Native
  # checkout (useful for development against an unreleased branch). When unset,
  # FetchContent manages third_party/maplibre-native and keeps it at
  # MLN_GIT_TAG.
  if(DEFINED MLN_SOURCE_DIR
     AND NOT MLN_SOURCE_DIR STREQUAL "")
    add_subdirectory("${MLN_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/maplibre-native")
  else()
    include(FetchContent)
    message(STATUS "Fetching MapLibre Native ${MLN_GIT_TAG}")
    fetchcontent_declare(
      maplibre_native
      GIT_REPOSITORY
      https://github.com/maplibre/maplibre-native.git
      GIT_TAG
      ${MLN_GIT_TAG}
      GIT_SHALLOW
      TRUE
      GIT_SUBMODULES_RECURSE
      TRUE
      SOURCE_DIR
      ${PROJECT_SOURCE_DIR}/third_party/maplibre-native)
    fetchcontent_makeavailable(maplibre_native)
    set(MLN_SOURCE_DIR "${maplibre_native_SOURCE_DIR}")
  endif()

  include("${MLN_SOURCE_DIR}/vendor/nunicode.cmake")
  include("${MLN_SOURCE_DIR}/vendor/sqlite.cmake")

  set(MLN_SOURCE_DIR "${MLN_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
