function(mln_ffi_configure_options)
  set(MLN_FFI_RENDER_BACKENDS ""
      CACHE STRING "Render backends that the library carries, as a list")
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER ""
      CACHE STRING "OpenGL context provider, for a build that carries OpenGL")
  set_property(
    CACHE MLN_FFI_OPENGL_CONTEXT_PROVIDER
    PROPERTY STRINGS egl wgl webgl)
  set(MLN_FFI_EGL_ROOT "" CACHE PATH "Path to a local EGL/GLES package")
  set(MLN_FFI_DEFAULT_RENDER_BACKEND ""
      CACHE STRING "Render backend that bindings take for this target")
  set_property(
    CACHE MLN_FFI_DEFAULT_RENDER_BACKEND
    PROPERTY STRINGS metal opengl vulkan webgpu)
  set(MLN_FFI_TEST_BACKENDS ""
      CACHE
        STRING
        "Render backends whose native suites the build compiles, as a list; empty means every backend the library carries"
  )
  set(MLN_FFI_RUST_TARGET "" CACHE STRING "Rust target triple for this build")
  set(MLN_FFI_TARGET_PLATFORM ""
      CACHE STRING "Platform name that bindings match this artifact by")
  set(MLN_FFI_ZIG_TARGET "" CACHE STRING "Zig target triple for this build")
  set(required MLN_FFI_TARGET_PLATFORM MLN_FFI_RUST_TARGET
      MLN_FFI_RENDER_BACKENDS MLN_FFI_DEFAULT_RENDER_BACKEND)
  # Zig can't build for the browser package, which is no library to link.
  if(NOT MLN_FFI_WEB)
    list(APPEND required MLN_FFI_ZIG_TARGET)
  endif()
  foreach(variable IN LISTS required)
    if(NOT ${variable})
      message(FATAL_ERROR "The build needs ${variable}, which the preset sets")
    endif()
  endforeach()

  string(TOLOWER "${MLN_FFI_RENDER_BACKENDS}" backends)
  string(TOLOWER "${MLN_FFI_OPENGL_CONTEXT_PROVIDER}" provider)
  string(TOLOWER "${MLN_FFI_DEFAULT_RENDER_BACKEND}" default_backend)
  string(TOLOWER "${MLN_FFI_TEST_BACKENDS}" test_backends)
  list(REMOVE_DUPLICATES backends)
  foreach(backend IN LISTS backends)
    if(NOT backend MATCHES "^(metal|opengl|vulkan|webgpu)$")
      message(FATAL_ERROR "Unsupported render backend: ${backend}")
    endif()
  endforeach()

  if("metal" IN_LIST backends AND NOT APPLE)
    message(FATAL_ERROR "Metal builds require an Apple platform")
  endif()
  # mln-rs draws with Metal alone on Apple platforms.
  if(APPLE AND NOT backends STREQUAL "metal")
    message(FATAL_ERROR "Apple builds carry the Metal backend alone")
  endif()
  # The browser's WebGPU is the only one that a host hands the library a
  # device of: native hosts drive Vulkan, Metal, or OpenGL.
  if("webgpu" IN_LIST backends AND NOT MLN_FFI_WEB)
    message(FATAL_ERROR "WebGPU builds are web presets")
  endif()
  # A browser downloads the module, so each web package carries one backend.
  list(LENGTH backends backend_count)
  if(MLN_FFI_WEB AND NOT backend_count EQUAL 1)
    message(FATAL_ERROR "A web preset carries one render backend")
  endif()
  if("opengl" IN_LIST backends)
    if(NOT provider MATCHES "^(egl|wgl|webgl)$")
      message(FATAL_ERROR "Unsupported OpenGL context provider: ${provider}")
    endif()
    if(provider STREQUAL "wgl" AND NOT WIN32)
      message(FATAL_ERROR "OpenGL WGL builds require Windows")
    endif()
    if(provider STREQUAL "webgl" AND NOT MLN_FFI_WEB)
      message(FATAL_ERROR "OpenGL WebGL builds are web presets")
    endif()
    if(MLN_FFI_WEB AND NOT provider STREQUAL "webgl")
      message(
        FATAL_ERROR "Browser OpenGL builds require the webgl context provider")
    endif()
  elseif(provider)
    message(
      FATAL_ERROR
        "MLN_FFI_OPENGL_CONTEXT_PROVIDER is only valid for builds that carry OpenGL"
    )
  endif()
  # The default backend is the platform's. A native build carries it, and the
  # browser's WebGL package names the browser's default, WebGPU, which the
  # other package carries.
  if(NOT MLN_FFI_WEB AND NOT default_backend IN_LIST backends)
    message(
      FATAL_ERROR
        "The default render backend ${default_backend} is not one of the build's: ${backends}"
    )
  endif()
  if(NOT test_backends)
    set(test_backends ${backends})
  endif()
  foreach(backend IN LISTS test_backends)
    if(NOT backend IN_LIST backends)
      message(
        FATAL_ERROR
          "MLN_FFI_TEST_BACKENDS names ${backend}, which the build does not carry: ${backends}"
      )
    endif()
  endforeach()

  list(JOIN backends ", " described)
  if(provider)
    string(APPEND described " (OpenGL through ${provider})")
  endif()
  message(
    STATUS
      "Configuring maplibre-native-c for ${MLN_FFI_RUST_TARGET} with ${described}"
  )

  set(MLN_FFI_RENDER_BACKENDS "${backends}" PARENT_SCOPE)
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER "${provider}" PARENT_SCOPE)
  set(MLN_FFI_DEFAULT_RENDER_BACKEND "${default_backend}" PARENT_SCOPE)
  set(MLN_FFI_TEST_BACKENDS "${test_backends}" PARENT_SCOPE)
  set(MLN_FFI_EGL_ROOT "${MLN_FFI_EGL_ROOT}" PARENT_SCOPE)
endfunction()
