# Dart binding

`maplibre_native_ffi` is the low-level Dart binding for the public MapLibre
Native C API. The package exposes explicit native handle lifetimes, copied value
types, runtime event polling, resource callbacks, offline operations, and the
render backend descriptors used by host integrations.

## Build and test

The repository-level mise configuration installs Dart and the native build
toolchain. From the repository root:

```bash
mise run //bindings/dart:test
mise run //bindings/dart:test linux-x64-vulkan
mise run //bindings/dart:ffigen-check
```

The test task builds the selected CMake preset, loads the installed native
library, analyzes the package, and runs the Dart tests. The private raw
declarations are checked in so Git and pub package consumers receive a complete
library; `ffigen-check` regenerates them separately and verifies that the
committed file is current. Generation is configured in `tool/ffigen.dart`.

Applications may set `MLN_FFI_NATIVE_LIBRARY` to an absolute library path.
Otherwise the loader uses the platform library name and the host's normal
dynamic-library search path.

## Ownership and execution

Owned handles have an idempotent `close()` or `discard()` operation. Close child
maps, render sessions, frames, snapshots, request handles, and offline
operations before their parent runtime. Scoped backend values remain valid only
until their frame or owner is closed.

Runtime and map work is synchronous and owner-thread-affine. Keep a handle and
all calls that use it on the execution context that created it. Poll queued
callbacks and events with `RuntimeHandle.runOnce()` and
`RuntimeHandle.pollEvent()`.

Resource-request completion is one-shot. Calling `complete()` or `close()`
releases the provider reference even when completion reports a native error.
Callback exceptions are contained at the native boundary and reported through
the native diagnostic path.

Unsigned C `uint64_t` JSON values and feature identifiers use Dart `BigInt` so
the complete native range is preserved. Native buffers return copied bytes;
direct pointer access is explicitly unsafe and ends at `NativeBuffer.close()`.
