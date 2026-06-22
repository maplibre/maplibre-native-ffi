function(mln_json_escape out value)
  string(REPLACE "\\" "\\\\" escaped "${value}")
  string(REPLACE "\"" "\\\"" escaped "${escaped}")
  string(REPLACE "\n" "\\n" escaped "${escaped}")
  string(REPLACE "\r" "\\r" escaped "${escaped}")
  string(REPLACE "\t" "\\t" escaped "${escaped}")
  set("${out}" "${escaped}" PARENT_SCOPE)
endfunction()

function(mln_json_array out)
  set(items "")
  foreach(value IN LISTS ARGN)
    if(value STREQUAL "")
      continue()
    endif()
    mln_json_escape(escaped "${value}")
    list(APPEND items "\"${escaped}\"")
  endforeach()
  string(JOIN ", " joined ${items})
  set("${out}" "[${joined}]" PARENT_SCOPE)
endfunction()

function(mln_write_artifact_metadata target)
  set(include_dirs "${PROJECT_SOURCE_DIR}/include")
  set(library_dirs "")
  set(rpaths "")
  set(link_libraries "")
  set(frameworks "")
  set(supports_linker_rpath false)

  list(APPEND include_dirs "$ENV{MLN_FFI_DEPENDENCY_INCLUDE_DIR}")
  list(APPEND include_dirs "$ENV{MLN_FFI_VULKAN_INCLUDE_DIR}")
  if(NOT CMAKE_CROSSCOMPILING)
    list(APPEND library_dirs "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}")
  endif()

  if(MLN_FFI_EGL_ROOT)
    list(APPEND include_dirs "${MLN_FFI_EGL_ROOT}/include")
    list(APPEND library_dirs "${MLN_FFI_EGL_ROOT}")
  endif()

  if(UNIX AND MLN_FFI_ARTIFACT_SHAPE STREQUAL "shared-private")
    set(rpaths "$<TARGET_FILE_DIR:${target}>" ${library_dirs})
    set(supports_linker_rpath true)
  endif()

  if(MLN_FFI_ARTIFACT_SHAPE STREQUAL "static-monolithic")
    list(
      APPEND library_dirs "${CMAKE_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/maplibre-native"
      "${CMAKE_BINARY_DIR}/maplibre-native/vendor/maplibre-tile-spec/cpp")
    list(
      APPEND
      link_libraries
      maplibre-native-c
      mbgl-core
      mbgl-freetype
      mbgl-harfbuzz
      mbgl-vendor-csscolorparser
      mbgl-vendor-nunicode
      mbgl-vendor-parsedate
      mbgl-vendor-sqlite
      mlt-cpp)

    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
      list(APPEND link_libraries c++ objc sqlite3 z)
      list(
        APPEND
        frameworks
        CoreFoundation
        CoreGraphics
        CoreText
        Foundation
        ImageIO
        Metal
        MetalKit)
      if(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
        list(APPEND frameworks QuartzCore)
      endif()
    endif()
  endif()

  mln_json_escape(render_backend "${MLN_FFI_RENDER_BACKEND}")
  mln_json_escape(artifact_shape "${MLN_FFI_ARTIFACT_SHAPE}")
  mln_json_array(include_dirs_json ${include_dirs})
  mln_json_array(library_dirs_json ${library_dirs})
  mln_json_array(rpaths_json ${rpaths})
  mln_json_array(link_libraries_json ${link_libraries})
  mln_json_array(frameworks_json ${frameworks})

  file(
    GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/maplibre-native-c.dev.json"
    CONTENT
      "{
  \"render_backend\": \"${render_backend}\",
  \"artifact_shape\": \"${artifact_shape}\",
  \"library_path\": \"$<TARGET_FILE:${target}>\",
  \"import_library_path\": \"$<TARGET_LINKER_FILE:${target}>\",
  \"supports_linker_rpath\": ${supports_linker_rpath},
  \"include_dirs\": ${include_dirs_json},
  \"library_dirs\": ${library_dirs_json},
  \"rpaths\": ${rpaths_json},
  \"link_libraries\": ${link_libraries_json},
  \"frameworks\": ${frameworks_json}
}
")
endfunction()
