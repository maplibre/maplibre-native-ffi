# Components, rather than notice files, are registered on the targets that ship
# them. The same records produce the SBOM and the human-readable notice bundle.
function(mln_ffi_add_component target name url license)
  cmake_parse_arguments(PARSE_ARGV 4 component "" "SOURCE;VERSION;PARENT"
                        "NOTICES")
  set(record "{}")
  foreach(field name url license)
    string(JSON record SET "${record}" "${field}" "\"${${field}}\"")
  endforeach()
  foreach(field SOURCE VERSION PARENT)
    string(TOLOWER "${field}" key)
    # CMake paths use forward slashes, including on Windows.
    string(JSON record SET "${record}" "${key}" "\"${component_${field}}\"")
  endforeach()
  set(notices "{}")
  while(component_NOTICES)
    list(POP_FRONT component_NOTICES source output_name)
    if(NOT EXISTS "${source}")
      message(FATAL_ERROR "${name}: notice does not exist: ${source}")
    endif()
    string(JSON notices SET "${notices}" "${output_name}" "\"${source}\"")
  endwhile()
  string(JSON record SET "${record}" notices "${notices}")
  set_property(TARGET ${target} APPEND PROPERTY MLN_FFI_COMPONENTS "${record}")
endfunction()

function(mln_ffi_add_core_components target)
  set(vendor "${MLN_FFI_SOURCE_DIR}/vendor")
  mln_ffi_add_component(
    ${target}
    maplibre-native
    https://github.com/maplibre/maplibre-native
    BSD-2-Clause
    SOURCE
    "${MLN_FFI_SOURCE_DIR}"
    NOTICES
    "${MLN_FFI_SOURCE_DIR}/LICENSES.core.md"
    maplibre-native.md)

  # Upstream's combined notice covers these components. A directory embedded in
  # the engine uses the engine commit; a submodule uses its own pinned commit.
  foreach(
    entry
    IN
    ITEMS
    "boost|boost|https://github.com/maplibre/maplibre-gl-native-boost|BSL-1.0"
    "csscolorparser|csscolorparser|https://github.com/mapbox/css-color-parser-cpp|MIT"
    "earcut.hpp|earcut.hpp|https://github.com/mapbox/earcut.hpp|ISC"
    "eternal|eternal|https://github.com/mapbox/eternal|ISC"
    "expected-lite|expected-lite|https://github.com/martinmoene/expected-lite|BSL-1.0"
    "kdbush.hpp|kdbush.hpp|https://github.com/mourner/kdbush.hpp|ISC"
    "maplibre-native-base|maplibre-native-base|https://github.com/maplibre/maplibre-native-base|BSD-3-Clause"
    "cheap-ruler-cpp|maplibre-native-base/deps/cheap-ruler-cpp|https://github.com/mapbox/cheap-ruler-cpp|ISC"
    "geojson-vt-cpp|maplibre-native-base/deps/geojson-vt-cpp|https://github.com/mapbox/geojson-vt-cpp|ISC"
    "geojson.hpp|maplibre-native-base/deps/geojson.hpp|https://github.com/mapbox/geojson-cpp|ISC"
    "geometry.hpp|maplibre-native-base/deps/geometry.hpp|https://github.com/mapbox/geometry.hpp|ISC"
    "shelf-pack-cpp|maplibre-native-base/deps/shelf-pack-cpp|https://github.com/mapbox/shelf-pack-cpp|ISC"
    "variant|maplibre-native-base/deps/variant|https://github.com/mapbox/variant|BSD-3-Clause"
    "parsedate|parsedate|https://github.com/curl/curl|curl"
    "polylabel|polylabel|https://github.com/mapbox/polylabel|ISC"
    "protozero|protozero|https://github.com/mapbox/protozero|BSD-2-Clause"
    "rapidjson|rapidjson|https://github.com/Tencent/rapidjson|MIT"
    "supercluster.hpp|supercluster|https://github.com/mapbox/supercluster.hpp|ISC"
    "unique_resource|unique_resource|https://github.com/okdshin/unique_resource|BSL-1.0"
    "vector-tile|vector-tile|https://github.com/maplibre/mvt-cpp|ISC"
    "wagyu|wagyu|https://github.com/mapbox/wagyu|BSL-1.0 AND BSD-3-Clause")
    string(REPLACE "|" ";" fields "${entry}")
    list(GET fields 0 name)
    list(GET fields 1 directory)
    list(GET fields 2 url)
    list(GET fields 3 license)
    mln_ffi_add_component(
      ${target}
      "${name}"
      "${url}"
      "${license}"
      SOURCE
      "${vendor}/${directory}"
      PARENT
      maplibre-native)
  endforeach()
  mln_ffi_add_component(
    ${target}
    unordered_dense
    https://github.com/martinus/unordered_dense
    MIT
    SOURCE
    "${vendor}/unordered_dense"
    PARENT
    maplibre-native
    NOTICES
    "${vendor}/unordered_dense/LICENSE"
    unordered_dense.txt)
  mln_ffi_add_component(
    ${target}
    icu
    https://github.com/unicode-org/icu
    ICU
    SOURCE
    "${vendor}/icu"
    PARENT
    maplibre-native
    NOTICES
    "${vendor}/icu/LICENSE"
    icu.txt)
  mln_ffi_add_component(
    ${target}
    nunicode
    https://github.com/alekseyt/nunicode
    MIT
    SOURCE
    "${vendor}/nunicode"
    PARENT
    maplibre-native
    NOTICES
    "${vendor}/nunicode/LICENSE"
    nunicode.txt)
  mln_ffi_add_component(
    ${target}
    sqlite
    https://sqlite.org
    blessing
    SOURCE
    "${vendor}/sqlite"
    PARENT
    maplibre-native)
  mln_ffi_add_component(
    ${target}
    maplibre-tile-spec
    https://github.com/maplibre/maplibre-tile-spec
    "MIT OR Apache-2.0"
    SOURCE
    "${vendor}/maplibre-tile-spec"
    PARENT
    maplibre-native
    NOTICES
    "${vendor}/maplibre-tile-spec/LICENSE-MIT"
    maplibre-tile-spec-mit.txt
    "${vendor}/maplibre-tile-spec/LICENSE-APACHE"
    maplibre-tile-spec-apache.txt)
  if(TARGET fastpfor-lib)
    mln_ffi_add_component(
      ${target}
      fastpfor
      https://github.com/fast-pack/FastPFOR
      Apache-2.0
      SOURCE
      "${vendor}/maplibre-tile-spec/cpp/vendor/fastpfor"
      PARENT
      maplibre-tile-spec
      NOTICES
      "${vendor}/maplibre-tile-spec/LICENSE-APACHE"
      fastpfor.txt)
  endif()
  if(MLN_WITH_PMTILES)
    mln_ffi_add_component(
      ${target}
      pmtiles
      https://github.com/protomaps/PMTiles
      BSD-3-Clause
      SOURCE
      "${vendor}/PMTiles"
      PARENT
      maplibre-native
      NOTICES
      "${vendor}/PMTiles/LICENSE"
      pmtiles.txt)
  endif()
  if(MLN_TEXT_SHAPING_HARFBUZZ)
    mln_ffi_add_component(
      ${target}
      freetype
      https://github.com/freetype/freetype
      FTL
      SOURCE
      "${vendor}/freetype"
      PARENT
      maplibre-native)
    mln_ffi_add_component(
      ${target}
      harfbuzz
      https://github.com/harfbuzz/harfbuzz
      MIT
      SOURCE
      "${vendor}/harfbuzz"
      PARENT
      maplibre-native)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    mln_ffi_add_component(
      ${target}
      jni.hpp
      https://github.com/mapbox/jni.hpp
      ISC
      SOURCE
      "${vendor}/maplibre-native-base/deps/jni.hpp"
      PARENT
      maplibre-native
      NOTICES
      "${vendor}/maplibre-native-base/deps/jni.hpp/LICENSE.txt"
      jni.hpp.txt)
  endif()
  if(MLN_WITH_METAL)
    mln_ffi_add_component(
      ${target}
      metal-cpp
      https://github.com/apple/metal-cpp
      Apache-2.0
      SOURCE
      "${vendor}/metal-cpp"
      PARENT
      maplibre-native)
  endif()
endfunction()

function(mln_ffi_install_components target artifact_file)
  set(records)
  foreach(component_target ${target} mln_ffi_platform_dependencies
          mln_ffi_render_dependencies)
    get_target_property(components ${component_target} MLN_FFI_COMPONENTS)
    if(components)
      list(APPEND records ${components})
    endif()
  endforeach()
  list(JOIN records ",\n" records)
  set(input "${CMAKE_CURRENT_BINARY_DIR}/components.json")
  file(WRITE "${input}" "[${records}]\n")
  set(output "${CMAKE_CURRENT_BINARY_DIR}/components")
  set(arguments)
  if(TARGET mln_ffi_platform_rust)
    get_target_property(rust_about mln_ffi_platform_rust MLN_FFI_CARGO_ABOUT)
    list(APPEND arguments --cargo-about "${rust_about}" --rust-target
         "${MLN_FFI_RUST_TARGET}")
  endif()
  find_program(MLN_FFI_PYTHON NAMES python3 REQUIRED)
  add_custom_target(
    ${target}_components
    COMMAND
      "${MLN_FFI_PYTHON}"
      "${PROJECT_SOURCE_DIR}/tools/release/sbom.py"
      native
      --records
      "${input}"
      --artifact
      "${artifact_file}"
      --output
      "${output}"
      ${arguments}
    BYPRODUCTS "${output}/sbom.cdx.json"
    VERBATIM)
  add_dependencies(${target}_components ${target}_artifact_descriptor)
  add_dependencies(${target} ${target}_components)
  install(
    DIRECTORY "${output}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT native)
  set_property(
    TARGET ${target}
    PROPERTY MLN_FFI_COMPONENT_DIRECTORY "${output}")
endfunction()
