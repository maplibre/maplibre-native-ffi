# Dart binding

`maplibre_native_ffi` is the low-level Dart binding for the public MapLibre
Native C API. The package exposes explicit native handle lifetimes, copied value
types, owned runtime event batches, ordinary futures, resource callbacks, and
the render backend descriptors used by host integrations.

## Build and test

The repository-level mise configuration installs Dart and the native build
toolchain. From the repository root:

```bash
mise run //bindings/dart:test
mise run //bindings/dart:test linux-x64-vulkan
mise run //bindings/dart:build:mobile android-arm64-egl
mise run //bindings/dart:build:mobile ios-arm64-metal
mise run //bindings/dart:build:mobile ios-simulator-arm64-metal
mise run --force //bindings/dart:ffigen
```

The test task builds the selected CMake preset, points the build hook at the
resulting install prefix, analyzes the package, and runs the Dart tests. The
private raw declarations are checked in so Git and pub package consumers receive
a complete library; CI regenerates them and fails on any diff. Generation is
configured in `tool/ffigen.dart`.

The mobile build task creates a temporary Flutter host, builds the selected
native preset, and verifies that Flutter packages its code asset. Device and
simulator iOS use separate presets because their dynamic libraries target
different Apple SDKs.

The native library reaches Dart as a code asset that `hook/build.dart` declares,
which is how the generated `@Native` declarations resolve it. Build hooks run in
a semi-hermetic environment that strips arbitrary environment variables, so the
hook reads the install prefix from `.dart_tool/maplibre_native_install_dir`
rather than from an environment variable; the mise tasks write it. Without that
file the hook downloads the artifact matching the target from the snapshot
release, which is what a consumer taking this package as a git dependency gets.

Dart runs the hook for `dart run` as well as `dart test`, so regenerating the
bindings resolves a library it never calls. Run the test task first and the
pointer already names a local build; on its own,
`mise run //bindings/dart:ffigen` downloads one.

## Android host integration

The code asset carries the library on Android as it does everywhere else, so an
Android application packages the ABI it points the hook at and the platform
loader is never asked for one by name. What Android needs on top of the library
is the patched Rustls platform-verifier helper that the native TLS stack calls
over JNI, without which HTTPS requests cannot validate against the platform
trust policy:

```bash
mise run //bindings/rustls-platform-verifier-android:build
```

The application binds nothing in the helper, which uses a MapLibre FFI-private
Java package so it can coexist with another library that packages the upstream
Rustls helper, and the AAR carries the R8 keep rule the helper needs. The host
calls `mln_android_init` with its JNI environment, class, and application
context before creating a runtime.

The Kotlin runtime AARs bundle that helper together with
`libmaplibre-native-c.so` for each ABI. A Dart host packages the library itself,
so it takes the helper AAR alone rather than a runtime AAR carrying a second
copy of the same library.

## Ownership and execution

Runtime and map handles have an idempotent `close()`. Runtime close remains
asynchronous in Dart so callback roots stay alive through native teardown. Close
child maps, render sessions, frames, snapshots, request handles, and offline
operations before their parent runtime. Scoped backend values remain valid only
until their frame or owner is closed.

Projection handles are created asynchronously and are synchronous after that:
every projection call, `close()` included, runs on the calling isolate's thread,
may be made from any isolate, and never observes map changes made after creation
and remains usable after its source map and runtime close.

Create runtimes and maps with `await`. Runtime and map commands copy their input
and return `Future` values. Snapshot methods synchronously copy immutable state.
Ordered queries and lifecycle operations also return `Future` values. Direct
wake callbacks report queued events without participating in future completion.
Read queued events with `RuntimeHandle.drainEvents()`. Narrow what a map or a
runtime queues with `setEventMask`.

Runtime, map, camera, and projection calls remain valid when Dart resumes an
isolate on another native thread after `await`. Attach a render session directly
from its map on the isolate that will own the graphics session.

Resource-request completion is one-shot. Calling `complete()` or `close()`
releases the provider reference even when completion reports a native error.
Callback exceptions are contained at the native boundary and reported through
the native diagnostic path.

Unsigned C `uint64_t` JSON values, feature identifiers, and camera transition
IDs use Dart `BigInt` so the complete native range is preserved. Native buffers
return copied bytes; direct pointer access is explicitly unsafe and ends at
`NativeBuffer.close()`.
