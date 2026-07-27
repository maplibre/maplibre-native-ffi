function(mln_add_license target source output_name)
  if(NOT TARGET ${target})
    message(
      FATAL_ERROR "Cannot register a license for missing target ${target}")
  endif()
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "License source does not exist: ${source}")
  endif()

  set_property(
    TARGET ${target}
    APPEND
    PROPERTY MLN_FFI_LICENSE_FILES "${source}")
  set_property(
    TARGET ${target}
    APPEND
    PROPERTY MLN_FFI_LICENSE_NAMES "${output_name}")
endfunction()

function(mln_add_core_licenses target)
  mln_add_license(${target} "${MLN_SOURCE_DIR}/LICENSES.core.md"
                  "maplibre-native.md")
  mln_add_license(${target} "${MLN_SOURCE_DIR}/vendor/icu/LICENSE" "icu.txt")
  mln_add_license(${target} "${MLN_SOURCE_DIR}/vendor/nunicode/LICENSE"
                  "nunicode.txt")
  if(MLN_WITH_PMTILES)
    mln_add_license(${target} "${MLN_SOURCE_DIR}/vendor/PMTiles/LICENSE"
                    "pmtiles.txt")
  endif()
endfunction()

function(mln_install_licenses target component)
  set(license_targets ${target} mln_ffi_platform_dependencies
      mln_ffi_render_dependencies)
  if(TARGET maplibre_native_platform_rust)
    list(APPEND license_targets maplibre_native_platform_rust)
  endif()

  set(installed_names)
  foreach(license_target IN LISTS license_targets)
    get_target_property(license_files ${license_target} MLN_FFI_LICENSE_FILES)
    get_target_property(license_names ${license_target} MLN_FFI_LICENSE_NAMES)
    if(NOT license_files OR license_files MATCHES "-NOTFOUND$")
      continue()
    endif()

    list(LENGTH license_files file_count)
    list(LENGTH license_names name_count)
    if(NOT file_count EQUAL name_count)
      message(
        FATAL_ERROR
          "${license_target} has ${file_count} license files and ${name_count} output names")
    endif()

    math(EXPR last_index "${file_count} - 1")
    foreach(index RANGE ${last_index})
      list(GET license_files ${index} license_file)
      list(GET license_names ${index} license_name)
      if(license_name IN_LIST installed_names)
        message(FATAL_ERROR "Duplicate installed license name: ${license_name}")
      endif()
      list(APPEND installed_names "${license_name}")
      install(
        FILES "${license_file}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c/licenses"
        RENAME "${license_name}"
        COMPONENT "${component}")
    endforeach()
  endforeach()
endfunction()
