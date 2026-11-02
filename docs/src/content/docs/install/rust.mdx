---
title: Rust
description: Add the Rust crate to a project.
---

## Install

Add the crate from [crates.io](https://crates.io/crates/maplibre-native-ffi). It
needs Rust 1.85 or later.

```bash frame="terminal"
cargo add maplibre-native-ffi
```

When it builds, the crate downloads the C library for your target and links it
statically.

## Choose a backend

The crate uses your platform's
[default backend](/maplibre-native-ffi/install/#choose-a-backend). To choose
another, enable one feature: `opengl`, `metal`, `vulkan`, or `webgpu`. A feature
on a target-specific dependency applies only to that target:

```toml title="Cargo.toml"
[dependencies]
maplibre-native-ffi = "{{version}}"

[target.'cfg(target_os = "android")'.dependencies]
maplibre-native-ffi = { version = "{{version}}", features = ["vulkan"] }
```

## Windows with MinGW

For `x86_64-pc-windows-gnu`, link with MSYS2's MINGW64 Clang and LLD
(`-C link-arg=-fuse-ld=lld`), and keep MINGW64's `bin` directory on `PATH` for
its runtime DLLs.

## Browser

A browser build targets `wasm32-unknown-emscripten`. It needs nightly Rust to
build the standard library with atomics, and link options from the C library.
The [`maplibre-native-ffi-sys` docs](https://docs.rs/maplibre-native-ffi-sys)
list them.

## Android

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```rust title="src/lib.rs"
unsafe { maplibre_native_ffi::init_android(jni_env, context)? };
```

## Snapshots

The crate from crates.io downloads the C library of its own release. A Git
dependency has no release to download from, so a snapshot takes two steps.
First, depend on the crate from Git at the `unstable-native-snapshot` tag, which
points at the commit that the snapshot was built from:

```toml title="Cargo.toml"
[dependencies]
maplibre-native-ffi = { git = "https://github.com/maplibre/maplibre-native-ffi", tag = "unstable-native-snapshot" }
```

Then download the C library archive for your target from the
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot),
unpack it, and [use it as your own build](#use-your-own-build). Each snapshot
replaces the previous one, so to move to a newer one, run `cargo update` and
download its archive.

## Use your own build

To link a C library that you built or downloaded, set
`MAPLIBRE_NATIVE_FFI_LOCAL_BUILD` to the absolute path of its install prefix. It
must be built for the same target. A Git or path dependency on the crate always
needs one.

```toml title=".cargo/config.toml"
[env]
MAPLIBRE_NATIVE_FFI_LOCAL_BUILD = { value = "../maplibre-native-ffi/build/macos-arm64-metal/install", relative = true }
```

## Download from a mirror

If your builds can't reach GitHub, copy the release's archives to a server of
your own, in a directory named after the release tag, and set
`MAPLIBRE_NATIVE_FFI_RELEASE_URL` to the server's base URL. The crate still
checks each archive against the digest that it pinned at release.
