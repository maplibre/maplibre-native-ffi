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

## Android host integration

The Dart package does not yet publish Android artifacts. An Android application
that embeds this repository builds the native package and the local Rustls
platform-verifier helper:

```bash
mise run //bindings/rustls-platform-verifier-android:build
```

```kotlin
// app/build.gradle.kts
val maplibreNativeFfi = file("../maplibre-native-ffi")

dependencies {
  implementation(
    files(
      maplibreNativeFfi.resolve(
        "bindings/rustls-platform-verifier-android/build/outputs/aar/" +
          "rustls-platform-verifier-android-release.aar"
      )
    )
  )
}
```

The helper build runs the repository's mise dependency acquisition first. Mise
pins, acquires, and patches the verifier source used by both Cargo and Gradle.
The helper uses a MapLibre FFI-private Java package, so it can coexist with
another library that packages the upstream Rustls helper. The host must also
call `mln_android_init` with its JNI environment, class, and application context
before creating a runtime.

## Ownership and execution

Owned handles have an idempotent `close()` or `discard()` operation. Close child
maps, render sessions, frames, snapshots, request handles, and offline
operations before their parent runtime. Scoped backend values remain valid only
until their frame or owner is closed.

Runtime and map work is synchronous and owner-thread-affine. Keep a handle and
all calls that use it on the isolate that created it. Poll queued callbacks and
events with `RuntimeHandle.pump()` and `RuntimeHandle.pollEvent()`.

A render session is the exception: it belongs to the isolate that attached it,
which need not be the map's. A `MapHandle` cannot cross isolates, so
`MapHandle.attachRef()` produces a `MapAttachRef` that can. It carries the
native address and attaches; every other map call stays on the map's isolate.

## Known draft deviation: do not await in an isolate that holds a handle

The C API keys owner-thread checks on the OS thread. This binding keys them on
`Isolate.current.hashCode`, and the two are not equivalent: the Dart VM moves an
isolate between OS threads, and it does so when an isolate resumes from awaited
I/O. The isolate hash does not change, so the binding's own check still passes
while the native check starts failing.

Until that is addressed, do not `await` I/O on an isolate that holds a runtime,
map, projection, or render session. Create the handles, use them, and close them
without yielding to I/O in between. Dart offers no equivalent of Go's
`runtime.LockOSThread()`, so the binding cannot pin the isolate on your behalf.

Exceeding this produces `wrongThread` from every call on the handle, including
`close()`. Because close fails too, the native runtime is never destroyed and
`mln_runtime_destroy` refuses for the rest of the process.

Tracked in [#412](https://github.com/maplibre/maplibre-native-ffi/issues/412).

Resource-request completion is one-shot. Calling `complete()` or `close()`
releases the provider reference even when completion reports a native error.
Callback exceptions are contained at the native boundary and reported through
the native diagnostic path.

Unsigned C `uint64_t` JSON values, feature identifiers, and camera transition
IDs use Dart `BigInt` so the complete native range is preserved. Native buffers
return copied bytes; direct pointer access is explicitly unsafe and ends at
`NativeBuffer.close()`.
