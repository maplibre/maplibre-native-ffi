function(mln_ffi_configure_options)
  set(MLN_FFI_RENDER_BACKENDS ""
      CACHE
        STRING
        "Render backends that the library carries, as a list, by preference")
  set(MLN_FFI_EGL_ROOT "" CACHE PATH "Path to a local EGL/GLES package")
  set(MLN_FFI_DEFAULT_RENDER_BACKEND ""
      CACHE
        STRING
        "For a web preset, the backend whose package bindings take by default")
  set(MLN_FFI_RUST_TARGET "" CACHE STRING "Rust target triple for this build")
  set(MLN_FFI_TARGET_PLATFORM ""
      CACHE STRING "Platform name that bindings match this artifact by")
  set(MLN_FFI_ZIG_TARGET "" CACHE STRING "Zig target triple for this build")
  set(required MLN_FFI_TARGET_PLATFORM MLN_FFI_RUST_TARGET
      MLN_FFI_RENDER_BACKENDS)
  # Zig can't build for the browser package, which is no library to link. The
  # browser has a package for each backend, so its presets name the one that
  # bindings take by default.
  if(MLN_FFI_WEB)
    list(APPEND required MLN_FFI_DEFAULT_RENDER_BACKEND)
  else()
    list(APPEND required MLN_FFI_ZIG_TARGET)
  endif()
  foreach(variable IN LISTS required)
    if(NOT ${variable})
      message(FATAL_ERROR "The build needs ${variable}, which the preset sets")
    endif()
  endforeach()

  string(TOLOWER "${MLN_FFI_RENDER_BACKENDS}" backends)
  list(REMOVE_DUPLICATES backends)
  foreach(backend IN LISTS backends)
    if(NOT backend MATCHES "^(metal|opengl|vulkan|d3d12|webgpu)$")
      message(FATAL_ERROR "Unsupported render backend: ${backend}")
    endif()
  endforeach()

  if("metal" IN_LIST backends AND NOT APPLE)
    message(FATAL_ERROR "Metal builds require an Apple platform")
  endif()
  # mln-rs draws with Metal alone on Apple platforms. A web preset configured on
  # a Mac has no toolchain of its own, so CMake reports the host as APPLE.
  if(APPLE AND NOT MLN_FFI_WEB AND NOT backends STREQUAL "metal")
    message(FATAL_ERROR "Apple builds carry the Metal backend alone")
  endif()
  if("d3d12" IN_LIST backends AND (NOT WIN32 OR MLN_FFI_WEB))
    message(FATAL_ERROR "D3D12 builds require Windows")
  endif()
  # The browser's WebGPU is the only one that a host hands the library a
  # device of: native hosts drive Vulkan, Metal, D3D12, or OpenGL.
  if("webgpu" IN_LIST backends AND NOT MLN_FFI_WEB)
    message(FATAL_ERROR "WebGPU builds are web presets")
  endif()
  # A browser downloads the module, so each web package carries one backend.
  list(LENGTH backends backend_count)
  if(MLN_FFI_WEB AND NOT backend_count EQUAL 1)
    message(FATAL_ERROR "A web preset carries one render backend")
  endif()

  # OpenGL's contexts come from the platform: WebGL2 in the browser, WGL on
  # Windows, and EGL elsewhere.
  set(provider "")
  if("opengl" IN_LIST backends)
    if(MLN_FFI_WEB)
      set(provider webgl)
    elseif(WIN32)
      set(provider wgl)
    else()
      set(provider egl)
    endif()
  endif()

  # A native package carries every backend of its platform, so its default is
  # the one it prefers, which it lists first, as the library's device does. The
  # browser's WebGL package names the browser's default, WebGPU, which the
  # other package carries.
  if(MLN_FFI_WEB)
    string(TOLOWER "${MLN_FFI_DEFAULT_RENDER_BACKEND}" default_backend)
  else()
    list(GET backends 0 default_backend)
  endif()

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
  set(MLN_FFI_EGL_ROOT "${MLN_FFI_EGL_ROOT}" PARENT_SCOPE)
endfunction()
