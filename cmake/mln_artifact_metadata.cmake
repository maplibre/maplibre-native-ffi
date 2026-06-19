function(mln_json_escape out value)
  string(REPLACE "\\" "\\\\" escaped "${value}")
  string(REPLACE "\"" "\\\"" escaped "${escaped}")
  string(REPLACE "\n" "\\n" escaped "${escaped}")
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

function(mln_list_append_env_path out name)
  set(paths ${${out}})
  if(DEFINED ENV{${name}}
     AND NOT "$ENV{${name}}" STREQUAL "")
    list(APPEND paths "$ENV{${name}}")
  endif()
  set("${out}" "${paths}" PARENT_SCOPE)
endfunction()

function(mln_target_arch out)
  if(CMAKE_OSX_ARCHITECTURES)
    set(arch "${CMAKE_OSX_ARCHITECTURES}")
  elseif(ANDROID_ABI)
    set(arch "${ANDROID_ABI}")
  elseif(OHOS_ARCH)
    set(arch "${OHOS_ARCH}")
  else()
    set(arch "${CMAKE_SYSTEM_PROCESSOR}")
  endif()
  set("${out}" "${arch}" PARENT_SCOPE)
endfunction()

function(mln_artifact_metadata_path out)
  set("${out}" "${CMAKE_BINARY_DIR}/maplibre-native-c.dev.json" PARENT_SCOPE)
endfunction()

function(mln_write_artifact_metadata target)
  set(public_include_dirs "${PROJECT_SOURCE_DIR}/include")
  set(binding_include_dirs "${public_include_dirs}")
  set(dependency_library_dirs "")
  set(runtime_search_paths "")
  set(static_library_dirs "")
  set(static_libraries "")
  set(static_system_libraries "")
  set(static_frameworks "")

  mln_list_append_env_path(binding_include_dirs MLN_FFI_DEPENDENCY_INCLUDE_DIR)
  mln_list_append_env_path(binding_include_dirs MLN_FFI_VULKAN_INCLUDE_DIR)
  mln_list_append_env_path(dependency_library_dirs
                           MLN_FFI_DEPENDENCY_LIBRARY_DIR)

  if(MLN_FFI_EGL_ROOT)
    list(APPEND binding_include_dirs "${MLN_FFI_EGL_ROOT}/include")
    list(APPEND dependency_library_dirs "${MLN_FFI_EGL_ROOT}")
  endif()

  if(UNIX AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(runtime_search_paths ${dependency_library_dirs})
  endif()

  if(MLN_FFI_ARTIFACT_SHAPE STREQUAL "static-monolithic")
    list(
      APPEND static_library_dirs "${CMAKE_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/maplibre-native"
      "${CMAKE_BINARY_DIR}/maplibre-native/vendor/maplibre-tile-spec/cpp")
    list(
      APPEND
      static_libraries
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
      list(APPEND static_system_libraries objc sqlite3 z)
      list(
        APPEND
        static_frameworks
        CoreFoundation
        CoreGraphics
        CoreText
        Foundation
        ImageIO
        MetalKit)
    endif()
  endif()

  mln_target_arch(target_arch)
  mln_json_escape(variant "$ENV{MISE_ENV}")
  mln_json_escape(target_os "${CMAKE_SYSTEM_NAME}")
  mln_json_escape(target_arch "${target_arch}")
  mln_json_escape(render_backend "${MLN_FFI_RENDER_BACKEND}")
  mln_json_escape(opengl_context_provider "${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
  mln_json_escape(artifact_shape "${MLN_FFI_ARTIFACT_SHAPE}")
  mln_json_array(public_include_dirs_json ${public_include_dirs})
  mln_json_array(binding_include_dirs_json ${binding_include_dirs})
  mln_json_array(dependency_library_dirs_json ${dependency_library_dirs})
  mln_json_array(runtime_search_paths_json ${runtime_search_paths})
  mln_json_array(static_library_dirs_json ${static_library_dirs})
  mln_json_array(static_libraries_json ${static_libraries})
  mln_json_array(static_system_libraries_json ${static_system_libraries})
  mln_json_array(static_frameworks_json ${static_frameworks})

  mln_artifact_metadata_path(metadata_path)
  file(
    GENERATE
    OUTPUT "${metadata_path}"
    CONTENT
      "{
  \"schema_version\": 1,
  \"variant\": \"${variant}\",
  \"target_os\": \"${target_os}\",
  \"target_arch\": \"${target_arch}\",
  \"render_backend\": \"${render_backend}\",
  \"opengl_context_provider\": \"${opengl_context_provider}\",
  \"artifact_shape\": \"${artifact_shape}\",
  \"core_library_path\": \"$<TARGET_FILE:${target}>\",
  \"windows_import_library_path\": \"$<TARGET_LINKER_FILE:${target}>\",
  \"public_include_dirs\": ${public_include_dirs_json},
  \"binding_include_dirs\": ${binding_include_dirs_json},
  \"dependency_library_dirs\": ${dependency_library_dirs_json},
  \"runtime_search_paths\": ${runtime_search_paths_json},
  \"static_library_dirs\": ${static_library_dirs_json},
  \"static_libraries\": ${static_libraries_json},
  \"static_system_libraries\": ${static_system_libraries_json},
  \"static_frameworks\": ${static_frameworks_json},
  \"c_abi_version\": 0
}
")
endfunction()
