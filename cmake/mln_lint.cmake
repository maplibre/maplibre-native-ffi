function(mln_configure_source_linting target)
  if(NOT MLN_FFI_ENABLE_CLANG_TIDY)
    return()
  endif()

  find_program(MLN_FFI_CLANG_TIDY clang-tidy REQUIRED)

  set(MLN_FFI_CLANG_TIDY_COMMAND ${MLN_FFI_CLANG_TIDY} --quiet
      --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy)

  if(APPLE)
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-resource-dir
      OUTPUT_VARIABLE
        MLN_FFI_CLANG_RESOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
      COMMAND_ERROR_IS_FATAL ANY)
    list(APPEND MLN_FFI_CLANG_TIDY_COMMAND
         --extra-arg=-resource-dir=${MLN_FFI_CLANG_RESOURCE_DIR})
  endif()

  set_target_properties(
    ${target}
    PROPERTIES CXX_CLANG_TIDY "${MLN_FFI_CLANG_TIDY_COMMAND}")

  if(APPLE)
    # clang-tidy 19 crashes when this checker visits Apple SDK Objective-C
    # declarations from Objective-C++ sources. Revisit when updating clang-tidy.
    set(MLN_FFI_OBJCXX_CLANG_TIDY_COMMAND ${MLN_FFI_CLANG_TIDY_COMMAND}
        --checks=-cppcoreguidelines-avoid-const-or-ref-data-members,-clang-analyzer-core.CallAndMessage)
    set_target_properties(
      ${target}
      PROPERTIES OBJCXX_CLANG_TIDY "${MLN_FFI_OBJCXX_CLANG_TIDY_COMMAND}")
  endif()
endfunction()

function(mln_configure_project_source source)
  if(MSVC)
    set(MLN_FFI_WARNINGS /W3 /WX /wd4005)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(MLN_FFI_WARNINGS -Wall -Wextra -Werror -Wno-macro-redefined)
  else()
    set(MLN_FFI_WARNINGS -Wall -Wextra -Werror)
  endif()

  set_source_files_properties(
    ${source}
    PROPERTIES COMPILE_OPTIONS "${MLN_FFI_WARNINGS}")
endfunction()

function(mln_target_project_sources target)
  target_sources(${target} PRIVATE ${ARGN})
  foreach(source IN LISTS ARGN)
    mln_configure_project_source(${source})
  endforeach()
endfunction()

function(mln_target_vendor_sources target)
  target_sources(${target} PRIVATE ${ARGN})
  set_source_files_properties(${ARGN} PROPERTIES SKIP_LINTING TRUE)
endfunction()
