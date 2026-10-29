function(mln_ffi_install_wasm_sbom target)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(output "${CMAKE_CURRENT_BINARY_DIR}/metadata")
  # Linking downloads the SDK ports whose notices the inventory includes.
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    BYPRODUCTS "${output}/sbom/maplibre-native-ffi.cdx.json"
    COMMAND
      "${Python3_EXECUTABLE}"
      "${MLN_FFI_ROOT}/tools/release/binding_sbom.py"
      wasm
      --prefix
      "${MLN_FFI_NATIVE_PREFIX}"
      --sdk
      "${EMSCRIPTEN_ROOT_PATH}"
      --output
      "${output}"
      ${ARGN}
    VERBATIM)
  set_property(
    TARGET ${target}
    APPEND
    PROPERTY
      LINK_DEPENDS
      "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/sbom.cdx.json"
      "${MLN_FFI_ROOT}/tools/release/binding_sbom.py"
      "${MLN_FFI_ROOT}/tools/release/sbom.py")
  install(DIRECTORY "${output}/" DESTINATION . COMPONENT ${target})
endfunction()
