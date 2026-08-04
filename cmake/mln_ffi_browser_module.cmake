# The prelinked browser module.
#
# Every other platform ships a library a host loads and calls through its own
# foreign-function interface. A browser host cannot do that: an Emscripten
# archive is only linkable by the emsdk version that produced it, and a host
# written in Kotlin, TypeScript, or any other language without a C toolchain has
# no link step to run. So the browser's distributable artifact is the linked
# module itself -- an ES module and its wasm -- and the export list is part of
# the contract rather than something each host rediscovers.
#
# See #37 phase D. This retires the browser half of TODO(browser-packaging).

function(mln_ffi_add_browser_module target api_target)
  if(NOT EMSCRIPTEN)
    return()
  endif()

  set(export_list "${CMAKE_CURRENT_BINARY_DIR}/${target}-exports.txt")
  # CMAKE_NM is the emsdk's llvm-nm, which reads the wasm archive the toolchain
  # just wrote; a host nm would not.
  add_custom_command(
    OUTPUT "${export_list}"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DMLN_FFI_NM=${CMAKE_NM}"
      "-DMLN_FFI_ARCHIVE=$<TARGET_FILE:${api_target}>"
      "-DMLN_FFI_OUTPUT=${export_list}"
      "-DMLN_FFI_EXTRA_EXPORTS=_malloc$<SEMICOLON>_free$<SEMICOLON>_mln_browser_entry_index$<SEMICOLON>_mln_browser_entry_total$<SEMICOLON>_mln_browser_invoke_here$<SEMICOLON>_mln_browser_dispatch_protocol$<SEMICOLON>_mln_browser_headers_digest$<SEMICOLON>_mln_browser_entry_slots$<SEMICOLON>_mln_browser_log_install$<SEMICOLON>_mln_browser_log_take_since$<SEMICOLON>_mln_browser_log_mark$<SEMICOLON>_mln_browser_dispatcher_create$<SEMICOLON>_mln_browser_dispatcher_create_with_canvases$<SEMICOLON>_mln_browser_dispatcher_submit$<SEMICOLON>_mln_browser_dispatcher_destroy$<SEMICOLON>_mln_browser_dispatcher_stop$<SEMICOLON>_mln_browser_dispatcher_take_completion$<SEMICOLON>_mln_browser_log_take_dropped$<SEMICOLON>_mln_browser_sync_provider_install$<SEMICOLON>_mln_browser_sync_provider_thunk$<SEMICOLON>_mln_browser_sync_transform_install$<SEMICOLON>_mln_browser_sync_transform_thunk$<SEMICOLON>_mln_browser_dispatcher_submit_task$<SEMICOLON>_mln_browser_webgl_context_create$<SEMICOLON>_mln_browser_webgl_context_create_here$<SEMICOLON>_mln_browser_webgl_context_destroy$<SEMICOLON>_mln_browser_webgl_context_destroy_here$<SEMICOLON>_mln_browser_webgl_canvas_resize$<SEMICOLON>_mln_browser_webgl_canvas_resize_here$<SEMICOLON>_mln_browser_webgl_texture_create$<SEMICOLON>_mln_browser_webgl_texture_create_here$<SEMICOLON>_mln_browser_webgl_texture_destroy$<SEMICOLON>_mln_browser_webgl_texture_destroy_here$<SEMICOLON>_mln_browser_webgl_read_pixels$<SEMICOLON>_mln_browser_webgl_read_pixels_here$<SEMICOLON>_mln_browser_webgl_present_texture$<SEMICOLON>_mln_browser_webgl_present_texture_here"
      -P
      "${PROJECT_SOURCE_DIR}/cmake/scripts/mln_ffi_browser_exports.cmake"
    DEPENDS
      "$<TARGET_FILE:${api_target}>"
      "${PROJECT_SOURCE_DIR}/cmake/scripts/mln_ffi_browser_exports.cmake"
    COMMENT "Collecting browser module exports"
    VERBATIM)
  add_custom_target(${target}_exports DEPENDS "${export_list}")

  # The generic call table. Generated from the headers rather than from the
  # linked module, so nothing here waits on a link that has not happened yet,
  # and
  # every entry is a checked call by name rather than a cast function pointer.
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(dispatch_table "${CMAKE_CURRENT_BINARY_DIR}/browser/dispatch_table.c")
  set(dispatch_generator
      "${PROJECT_SOURCE_DIR}/scripts/generate-browser-dispatch.py")
  file(GLOB_RECURSE public_headers CONFIGURE_DEPENDS
       "${PROJECT_SOURCE_DIR}/include/*.h")
  add_custom_command(
    OUTPUT "${dispatch_table}"
    COMMAND
      "${Python3_EXECUTABLE}" "${dispatch_generator}"
      "--clang=${CMAKE_C_COMPILER}"
      "--sysroot=$ENV{EMSDK}/upstream/emscripten/cache/sysroot"
      "--include=${PROJECT_SOURCE_DIR}/include" "${dispatch_table}"
    DEPENDS
      "${dispatch_generator}" "${PROJECT_SOURCE_DIR}/scripts/browser_abi.py"
      ${public_headers}
    COMMENT "Generating browser dispatch table"
    VERBATIM)

  # An executable with no main: the module is a library of C entry points, and
  # the host drives it.
  add_executable(
    ${target}
    "${PROJECT_SOURCE_DIR}/src/browser/module_entry.c"
    "${PROJECT_SOURCE_DIR}/src/browser/dispatch.c"
    "${PROJECT_SOURCE_DIR}/src/browser/log_queue.c"
    "${PROJECT_SOURCE_DIR}/src/browser/dispatcher.c"
    "${PROJECT_SOURCE_DIR}/src/browser/sync_callback.c"
    "${PROJECT_SOURCE_DIR}/src/browser/webgl_context.c"
    "${PROJECT_SOURCE_DIR}/src/browser/webgl_host.c"
    "${dispatch_table}")
  add_dependencies(${target} ${target}_exports)
  target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src")
  target_link_libraries(${target} PRIVATE ${api_target})

  # The public headers use C23 fixed-underlying-type enums, and linking the C
  # API
  # does not carry its language mode across, so this states it rather than
  # relying on the toolchain accepting them as an extension.
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

  target_link_options(
    ${target}
    PRIVATE
      --no-entry
      # A host imports this from a page or a worker, and pthreads reach it from
      # their own workers, so all three environments stay in the module.
      "-sENVIRONMENT=web,worker"
      # The factory shape the module contract promises: one ES module default
      # export returning a promise of the instance.
      -sMODULARIZE=1
      -sEXPORT_ES6=1
      -sEXPORT_NAME=createMaplibreNativeC
      # The function table starts exactly as large as the static call graph
      # needs, and growth is what lets anything be added to it at run time. Note
      # that a trampoline added on the page cannot be called from a MapLibre
      # worker -- see src/browser/log_queue.c -- so this serves same-agent uses
      # rather than host callbacks native invokes from its own threads.
      -sALLOW_TABLE_GROWTH=1
      # Descriptors cross as bytes, handles as 64-bit values, and a browser host
      # attaches its own canvas, so the heap views, the string helpers, and the
      # GL registry are all part of the contract. PThread is there for one
      # reason: the factory spawns the worker pool before it resolves, so a host
      # that rejects the instance it just built -- over a digest, a protocol, or
      # a missing entry point -- holds sixteen workers it has no other way to
      # release, and every retry would add sixteen more.
      "-sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU16,HEAPU32,HEAPF32,HEAPF64,GL,PThread,addFunction,removeFunction,UTF8ToString,stringToUTF8,lengthBytesUTF8"
      "-sEXPORTED_FUNCTIONS=@${export_list}"
      # A worker-owned render target draws into a canvas the page transferred to
      # it, which is the only way a thread that may block can also draw.
      -sOFFSCREENCANVAS_SUPPORT=1
      # 64-bit handles reach JavaScript as BigInt rather than being split, so a
      # host cannot silently truncate one.
      -sWASM_BIGINT=1
      # An optimized link otherwise renames wasm exports to one- and two-letter
      # names and leaves the glue to map them. That is invisible to a host
      # calling through the module object, but it hides the one place the
      # lowered signature of each entry point can be read: a struct-returning C
      # function becomes a hidden out-pointer parameter, and nothing else in the
      # artifact records that. Linking the exports library is emcc's supported
      # way to keep the names, and it is what lets the ABI manifest be derived
      # from the shipped module rather than modelled beside it.
      -lexports.js)
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${export_list}")

  # The manifest reads the module emcc just wrote, because the toolchain has
  # applied its own ABI by then and nothing here has to model it a second time.
  set(manifest "${CMAKE_CURRENT_BINARY_DIR}/browser/maplibre_native_c-abi.json")
  set(module_wasm "${CMAKE_CURRENT_BINARY_DIR}/browser/maplibre_native_c.wasm")
  set(manifest_generator
      "${PROJECT_SOURCE_DIR}/scripts/generate-browser-abi-manifest.py")
  add_custom_command(
    OUTPUT "${manifest}"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DMLN_FFI_PYTHON=${Python3_EXECUTABLE}"
      "-DMLN_FFI_CLANG=${CMAKE_C_COMPILER}"
      "-DMLN_FFI_SYSROOT=$ENV{EMSDK}/upstream/emscripten/cache/sysroot"
      "-DMLN_FFI_EMSCRIPTEN_DIR=$ENV{EMSDK}/upstream/emscripten"
      "-DMLN_FFI_GENERATOR=${manifest_generator}"
      "-DMLN_FFI_MODULE=${module_wasm}"
      "-DMLN_FFI_HEADER_DIR=${PROJECT_SOURCE_DIR}/include"
      "-DMLN_FFI_OUTPUT=${manifest}"
      -P
      "${PROJECT_SOURCE_DIR}/cmake/scripts/mln_ffi_browser_manifest.cmake"
    DEPENDS
      ${target} "${manifest_generator}"
      "${PROJECT_SOURCE_DIR}/scripts/browser_abi.py"
      "${PROJECT_SOURCE_DIR}/cmake/scripts/mln_ffi_browser_manifest.cmake"
    COMMENT "Generating browser ABI manifest"
    VERBATIM)
  add_custom_target(${target}_manifest ALL DEPENDS "${manifest}")

  # emcc writes the wasm beside the module, and the manifest describes both, so
  # all three travel together or a host cannot check what it loaded.
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/browser/maplibre_native_c.mjs"
    "${module_wasm}" "${export_list}" "${manifest}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/browser"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()
