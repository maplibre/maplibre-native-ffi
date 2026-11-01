---
title: Python
description: Add the Python package to a project.
---

## Install

Install the binding and one runtime from
[PyPI](https://pypi.org/project/maplibre-native-ffi/). It needs Python 3.12 or
later. The runtime holds the native library for your
[backend](/maplibre-native-ffi/install/#choose-a-backend):
`maplibre-native-ffi-runtime-opengl`, `-metal`, or `-vulkan`. Importing the
binding fails unless exactly one runtime is installed.

```bash frame="terminal"
pip install maplibre-native-ffi maplibre-native-ffi-runtime-vulkan
```

To choose the runtime by platform, give each one an environment marker:

```toml title="pyproject.toml"
[project]
dependencies = [
  "maplibre-native-ffi",
  "maplibre-native-ffi-runtime-metal; sys_platform == 'darwin'",
  "maplibre-native-ffi-runtime-opengl; sys_platform != 'darwin'",
]
```

## Windows with MinGW

The runtimes above use MSVC on Windows. For MinGW, install
`maplibre-native-ffi-runtime-opengl-mingw` or
`maplibre-native-ffi-runtime-vulkan-mingw` instead.

## Android

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```python title="main.py"
maplibre_native_ffi.api.init_android(jni_env, context)
```

## Snapshots

Install both packages from the
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot).
A snapshot is a development release of the next version, so pip needs `--pre`:

```bash frame="terminal"
pip install --pre maplibre-native-ffi maplibre-native-ffi-runtime-vulkan --find-links https://github.com/maplibre/maplibre-native-ffi/releases/expanded_assets/unstable-native-snapshot
```

With uv, name the snapshot release as a flat index for both packages:

```toml title="pyproject.toml"
[project]
dependencies = ["maplibre-native-ffi", "maplibre-native-ffi-runtime-vulkan"]

[[tool.uv.index]]
name = "maplibre-snapshots"
url = "https://github.com/maplibre/maplibre-native-ffi/releases/expanded_assets/unstable-native-snapshot"
format = "flat"
explicit = true

[tool.uv.sources]
maplibre-native-ffi = { index = "maplibre-snapshots" }
maplibre-native-ffi-runtime-vulkan = { index = "maplibre-snapshots" }
```

The snapshot release holds only the latest snapshot, so a lockfile that names an
older one fails to install once a newer one replaces it.
