# The prelinked browser module.
#
# Every other platform ships a library a host loads and calls through its own
# foreign-function interface. A browser host cannot do that: an Emscripten
# archive is only linkable by the emsdk version that produced it, and a host
# written in Kotlin has no link step to run. So the browser's distributable
# artifact is the linked module itself -- an ES module and its wasm.
#
# The module carries the Kotlin binding's Emscripten shim and boots the binding
# on the pthread -sPROXY_TO_PTHREAD gives main(), where blocking is legal.

function(mln_ffi_add_browser_module target api_target)
  if(NOT EMSCRIPTEN)
    return()
  endif()
  # The binding this module boots renders through WebGL.
  if(NOT MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    return()
  endif()

  set(shim_dir "${PROJECT_SOURCE_DIR}/bindings/kotlin/emscripten")
  set(host_library "${shim_dir}/mln_kotlin_host.js")
  set(pre_library "${shim_dir}/mln_kotlin_pre.js")

  set(export_list "${CMAKE_CURRENT_BINARY_DIR}/${target}-exports.txt")
  # CMAKE_NM is the emsdk's llvm-nm, which reads the wasm archive the toolchain
  # just wrote; a host nm would not.
  #
  # `_main` is not decoration: naming EXPORTED_FUNCTIONS at all makes emcc treat
  # a module without it as a reactor and skip main entirely, which is where the
  # binding is booted from (emsdk tools/link.py:918-929). The shim's own entry
  # points arrive through EMSCRIPTEN_KEEPALIVE, which emcc appends to this list
  # after the link (tools/emscripten.py:567-595).
  add_custom_command(
    OUTPUT "${export_list}"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DMLN_FFI_NM=${CMAKE_NM}"
      "-DMLN_FFI_ARCHIVE=$<TARGET_FILE:${api_target}>"
      "-DMLN_FFI_OUTPUT=${export_list}"
      "-DMLN_FFI_EXTRA_EXPORTS=_main$<SEMICOLON>_malloc$<SEMICOLON>_free"
      -P
      "${PROJECT_SOURCE_DIR}/cmake/scripts/mln_ffi_browser_exports.cmake"
    DEPENDS
      "$<TARGET_FILE:${api_target}>"
      "${PROJECT_SOURCE_DIR}/cmake/scripts/mln_ffi_browser_exports.cmake"
    COMMENT "Collecting browser module exports"
    VERBATIM)
  add_custom_target(${target}_exports DEPENDS "${export_list}")

  add_executable(
    ${target} "${shim_dir}/mln_kotlin_main.c"
    "${shim_dir}/mln_kotlin_callbacks.c" "${shim_dir}/mln_kotlin_webgl.c")
  add_dependencies(${target} ${target}_exports)
  # The same two dependency targets mln_ffi_install_emscripten_options() writes
  # into share/maplibre-native-c/emscripten-link-flags.txt, so a module linked
  # here and a module linked out of the install prefix carry one set of options.
  target_link_libraries(
    ${target}
    PRIVATE ${api_target} MLN_FFI::RenderDependencies)

  # The public headers use C23 fixed-underlying-type enums, and linking the C
  # API does not carry its language mode across.
  set_target_properties(
    ${target}
    PROPERTIES
      C_STANDARD
      23
      C_STANDARD_REQUIRED
      YES
      C_EXTENSIONS
      OFF
      OUTPUT_NAME
      maplibre_native_c
      SUFFIX
      .mjs
      RUNTIME_OUTPUT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/browser")

  # A host imports the module from a page or a worker, and pthreads reach it
  # from their own workers, so all three environments stay in it.
  #
  # EXPORT_ES6 is also what makes pthread workers type:'module' (emsdk
  # libpthread.js:37-39), which is what makes import() available inside one --
  # and the binding is imported into a pthread. PROXY_TO_PTHREAD is what gives
  # main() that pthread, so the binding may block and the host's event loop
  # stays free.
  #
  # Nothing is transferred to it at startup: the host has no canvas by the time
  # the proxied main thread is created, and the default selector would fail the
  # create (emsdk libpthread.js:712-716). Canvases are registered later.
  #
  # WASM_BIGINT keeps a 64-bit handle a BigInt rather than a pair of i32s, so a
  # host cannot silently truncate one.
  #
  # Linking the exports library is emcc's supported way to keep the wasm export
  # names, which an optimized link otherwise minifies to one and two letters
  # (emsdk tools/link.py:1501-1522). That is invisible to a host calling through
  # the module object and fatal to scripts/check-browser-exports.py, which reads
  # each entry point's lowered signature out of the shipped module.
  target_link_options(
    ${target}
    PRIVATE
      "-sENVIRONMENT=web,worker"
      -sMODULARIZE=1
      -sEXPORT_ES6=1
      -sEXPORT_NAME=createMaplibreNativeC
      -sPROXY_TO_PTHREAD
      # The thread this binding runs on is created during instantiation, which
      # is the only moment a canvas can be transferred to it. The pre-js
      # registers one under this name either way, so a host with no on-screen
      # map does not fail thread creation on a selector matching nothing.
      "-sOFFSCREENCANVASES_TO_PTHREAD=maplibre"
      -sOFFSCREENCANVAS_SUPPORT=1
      -sWASM_BIGINT=1
      "-sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU16,HEAPU32,HEAPF32,HEAPF64,GL,UTF8ToString,stringToUTF8,lengthBytesUTF8"
      "-sEXPORTED_FUNCTIONS=@${export_list}"
      "--js-library=${host_library}"
      "--pre-js=${pre_library}"
      -lexports.js)
  set_property(
    TARGET ${target}
    APPEND
    PROPERTY LINK_DEPENDS "${export_list}" "${host_library}" "${pre_library}")

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/browser/maplibre_native_c.mjs"
    "${CMAKE_CURRENT_BINARY_DIR}/browser/maplibre_native_c.wasm"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/browser"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()
