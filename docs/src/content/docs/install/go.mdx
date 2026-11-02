---
title: Go
description: Add the Go module to a project.
---

## Install

Add the module. It needs Go 1.24 or later, with cgo.

```bash frame="terminal"
go get github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre@v{{version}}
```

cgo finds the C library through pkg-config. Download and unpack the
[C library](/maplibre-native-ffi/install/c/) for your preset, then point
pkg-config at it before you build:

```bash frame="terminal"
export PKG_CONFIG_PATH=<prefix>/share/pkgconfig
```

## Windows

cgo links the C library with MSYS2's Clang: MINGW64 on x64, or CLANGARM64 on
ARM64. Set `CC` to that environment's `bin/clang.exe` and `PKG_CONFIG` to its
`bin/pkgconf.exe`. Put the C library's `bin` directory and the environment's
`bin` directory on `PATH`.

## Android

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```go title="main.go"
err := maplibre.InitAndroid(jniEnv, context)
```

## Snapshots

Take the module at the `unstable-native-snapshot` tag, which points at the
commit that the snapshot was built from, and the C library from the
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot):

```bash frame="terminal"
go get github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre@unstable-native-snapshot
```
