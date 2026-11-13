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
  get_target_property(rust_about ${target} MLN_FFI_CARGO_ABOUT)
  if(rust_about)
    list(APPEND arguments --cargo-about "${rust_about}" --rust-target
         "${MLN_FFI_RUST_TARGET}")
  endif()
  find_program(MLN_FFI_PYTHON NAMES python3 REQUIRED)
  add_custom_target(
    ${target}_components
    ALL
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
    COMMENT "Writing the SBOM and license notices"
    VERBATIM)
  add_dependencies(${target}_components ${target}_package_metadata)
  add_dependencies(${target} ${target}_components)
  install(
    DIRECTORY "${output}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT native)
  set_property(
    TARGET ${target}
    PROPERTY MLN_FFI_COMPONENT_DIRECTORY "${output}")
endfunction()
