# Khronos OpenGL Registry Headers

This directory contains the small Khronos header subset needed by the Windows
WGL OpenGL backend.

## Source

Copy these files from the Khronos OpenGL registry:

<https://github.com/KhronosGroup/OpenGL-Registry>

Required files:

- `api/GL/wgl.h` -> `include/GL/wgl.h`
- `api/GL/wglext.h` -> `include/GL/wglext.h`
- `api/GLES3/gl3.h` -> `include/GLES3/gl3.h`
- `api/GLES3/gl3platform.h` -> `include/GLES3/gl3platform.h`
- `api/KHR/khrplatform.h` -> `include/KHR/khrplatform.h`

## Why This Is Vendored

MapLibre Native's Windows WGL backend includes `GLES3/gl3.h` and `GL/wgl.h`,
and uses WGL extension declarations from `GL/wglext.h`. Upstream MapLibre
Native gets those headers from its vendored vcpkg
[`opengl-registry`](https://vcpkg.io/en/package/opengl-registry.html) port.

This project uses Pixi for native dependencies. Conda-forge provides the EGL
and [`libgles-devel`](https://prefix.dev/channels/conda-forge/packages/libgles-devel)
headers used by the Linux EGL profiles, but it does not provide an equivalent
`opengl-registry` package for `win-64`. The Windows SDK provides the base
WGL/OpenGL API, but not this Khronos registry header set.
